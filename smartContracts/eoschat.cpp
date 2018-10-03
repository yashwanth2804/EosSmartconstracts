#include <eosiolib/eosio.hpp>
#include <eosiolib/contract.hpp>
#include <eosiolib/print.hpp>
#include <string>

namespace eoschat {

    using namespace eosio;

    class eoschat : public contract
    {
    public:
        explicit eoschat(account_name self) : contract(self) {}

        //@abi action
        void signup(account_name account, std::string& username) {
            eosio_assert( is_account( account ), "Account does not exist");
            require_auth(account);

            user_index users(_self, scope_account);

            auto it = users.find(account);
            eosio_assert(it == users.end(), "User already exists");

            users.emplace(scope_account, [&](auto& user) {
                user.account_name = account;
                user.username = username;
            });
        }

        //@abi action
        void send(account_name from, account_name to, std::string message) {
            require_auth(from);

            user_index users(_self, scope_account);

            eosio_assert(users.find(from) != users.end(), "Sender's account is not registered as a user");
            eosio_assert(users.find(to) != users.end(), "Destination account is not registered");
            eosio_assert(message.size() < MAX_MESSAGE_SIZE, "Message length is above the 1 MiB limit");

            require_recipient(from, to);
        }

    private:
        //@abi table user i64
        struct user {
            account_name account_name;
            std::string username;

            uint64_t primary_key() const { return account_name; }

            EOSLIB_SERIALIZE(user, (account_name)(username))
        };

        static const size_t MAX_MESSAGE_SIZE = 1 << 20;
        static const account_name scope_account = N(eoschat);

        typedef multi_index<N(user), user> user_index;
    };

    EOSIO_ABI(eoschat, (signup)(send))
}
