#include <eosiolib/eosio.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/currency.hpp>
#include <eosiolib/action.hpp>
#include <eosiolib/transaction.hpp>
namespace eoschat {

    using namespace eosio;

    class deal : public contract
    {
    public:
        explicit deal(account_name self) : contract(self) {}

        //@abi action
        void init(
                account_name initiator,
                account_name executor,
                asset        quantity,
                std::string  conditions,
                account_name multisig,
                account_name escrow_agent
        ) {
            require_auth(initiator);

            //a lot of checks
            eosio_assert( is_account( executor ),     "executor account does not exist");
            eosio_assert( is_account( escrow_agent ), "escrow agent account does not exist");
            eosio_assert( is_account( multisig ),     "multisig account does not exist");

            eosio_assert(initiator    != executor,     "Initiator and executor are identical");
            eosio_assert(initiator    != escrow_agent, "Initiator and escrow agent are identical");
            eosio_assert(escrow_agent != executor,     "Escrow agent and executor are identical");

            eosio_assert(initiator    != multisig,     "Initiator and multisig are identical");
            eosio_assert(executor     != multisig,     "Executor and multisig are identical");
            eosio_assert(escrow_agent != multisig,     "Escrow agent and multisig are identical");

            eosio_assert(!conditions.empty(), "Conditions are empty");

            // transfer initiator's asset to multisig account
            action act(
                    permission_level{initiator, N(active)},
                    N(eosio.token),
                    N(transfer),
                    currency::transfer{initiator, multisig, quantity, ""}
            );
            act.send();

            deal_index deals(_self, scope_account);

            auto it = deals.find(multisig);
            eosio_assert(it == deals.end(), "Multisig account reuse");

            action proposed_trx_action(
                    std::vector<permission_level>{
                            {initiator,    N(active)},
                            {executor,     N(active)}
                    },
                    /*std::vector<permission_level>{
                            {initiator,    N(active)},
                            {executor,     N(active)},
                            {escrow_agent, N(active)}
                    },*/
                    N(eosio.token),
                    N(transfer),
                    currency::transfer{multisig, executor, quantity, ""});

            transaction trx(time_point_sec(now() + deal_proposal_duration));
            trx.actions.push_back(std::move(proposed_trx_action));

            name proposal_name{multisig};

            deals.emplace(scope_account, [&](auto& deal) {
                deal = {initiator,
                        executor,
                        quantity,
                        conditions,
                        multisig,
                        escrow_agent,
                        proposal_name};
            });

            dispatch_inline(
                    N(eosio.msig),
                    N(propose),
                    std::vector<permission_level>{{initiator, N(active)}},
                    std::make_tuple(
                            initiator,
                            proposal_name,
                            //std::vector<permission_level>{{multisig, N(active)}},
                            std::vector<permission_level>{
                                    {initiator,    N(active)},
                                    {executor,     N(active)}
                            },
                            trx
                    ));

            require_recipient(executor);
        }

        //@abi action
        void accept(account_name account, account_name key) {
            require_auth(account);

            deal_index deals(_self, scope_account);
            auto it = deals.find(key);

            eosio_assert(it != deals.end(), "No table entry for provided key");

            dispatch_inline(
                    N(eosio.msig),
                    N(approve),
                    std::vector<permission_level>{{account, N(active)}},
                    std::make_tuple(
                            it->initiator,
                            it->proposal_name,
                            permission_level{account, N(active)}
                    ));

            if (account == it->initiator || account == it->escrow_agent) {
                dispatch_inline(
                        N(eosio.msig),
                        N(exec),
                        std::vector < permission_level > {{account, N(active)}},
                        std::make_tuple(
                                it->initiator,
                                it->proposal_name,
                                account
                        ));

                require_recipient(it->initiator, it->executor);
            }
        }

        //@abi action
        void dispute(account_name account, account_name key) {
            require_auth(account);

            deal_index deals(_self, scope_account);
            auto it = deals.find(key);

            eosio_assert(it != deals.end(), "No table entry for provided key");

            if (account == it->executor) { //executor don't want to wait anymore
/*                dispatch_inline(
                        N(eoschat),
                        N(send),
                        std::vector<permission_level>{{_self, N(active)}},
                        std::make_tuple(
                                _self,
                                it->escrow_agent,
                                "Executor started dispute"
                        ));*/
            } else if (account == it->initiator) { //initiator wants his asset back
                dispatch_inline(
                        N(eosio.msig),
                        N(unapprove),
                        std::vector<permission_level>{{account, N(active)}},
                        std::make_tuple(
                                it->initiator,
                                it->proposal_name,
                                std::vector<permission_level>{{account, N(active)}}
                        ));

                deal_index deals(_self, scope_account);

                auto it = deals.find(key);
                eosio_assert(it != deals.end(), "No table entry");

                action proposed_trx_action(
                        permission_level{it->multisig, N(active)},
                        N(eosio.token),
                        N(transfer),
                        currency::transfer{it->multisig, it->initiator, it->quantity, ""});

                transaction trx(time_point_sec(now() + deal_proposal_duration));
                trx.actions.push_back(std::move(proposed_trx_action));

                dispatch_inline(
                        N(eosio.msig),
                        N(propose),
                        std::vector<permission_level>{{account, N(active)}},
                        std::make_tuple(
                                it->initiator,
                                convert_to_counter_proposal(it->proposal_name),
                                std::vector<permission_level>{{it->multisig, N(active)}},
                                trx
                        ));

                dispatch_inline(
                        N(eosio.msig),
                        N(approve),
                        std::vector<permission_level>{{account, N(active)}},
                        std::make_tuple(
                                it->initiator,
                                it->proposal_name,
                                std::vector<permission_level>{{account, N(active)}}
                        ));

/*                dispatch_inline(
                        N(eoschat),
                        N(send),
                        std::vector<permission_level>{{_self, N(active)}},
                        std::make_tuple(
                                _self,
                                it->escrow_agent,
                                "Initiator started dispute"
                        ));*/
            } else {
                abort();
            }

            require_recipient(it->escrow_agent);
        }

        //@abi action
        void choose(account_name agent, account_name winner, account_name key) {
            require_auth(agent);

            deal_index deals(_self, scope_account);
            auto it = deals.find(key);

            eosio_assert(it != deals.end(), "No table entry for provided key");
            eosio_assert(it->escrow_agent == agent, "Wrong agent or key");
            eosio_assert(it->initiator == winner || it->executor == winner, "Winner account does not exist");

            if (it->initiator == winner) {
                deals.modify(it, scope_account, [&](auto& deal) {
                    deal.proposal_name = convert_to_counter_proposal(deal.proposal_name);
                });
            }

            accept(winner, key);

            require_recipient(winner);
        }

        void clear(account_name key) {
            require_auth(_self);

            deal_index deals(_self, scope_account);
            auto it = deals.find(key);
            if (it != deals.end())
                deals.erase(it);
        }

    private:
        //@abi table deals
        struct deal_info {
            account_name initiator;
            account_name executor;
            asset        quantity;
            std::string  conditions;
            account_name multisig;
            account_name escrow_agent;
            name         proposal_name;

            account_name primary_key() const { return multisig; }

            EOSLIB_SERIALIZE(deal_info, (initiator)(executor)(quantity)(conditions)(multisig)(escrow_agent)(proposal_name))
        };

        typedef multi_index<N(deals), deal_info> deal_index;

        static const account_name scope_account = N(eoschat.deal);
        static const uint64_t deal_proposal_duration = 60*60*24*30; //seconds = 30 days

        name convert_to_counter_proposal(const name& proposal_name) {
            std::string counter_proposal_str{"counter."};
            counter_proposal_str += proposal_name.to_string();
            return name{string_to_name(counter_proposal_str.c_str())};
        }
    };

    EOSIO_ABI(deal, (init)(accept)(dispute)(choose)(clear))
}
#include <string>

#include <vector>
