#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
using namespace eosio;


class profile : public eosio::contract {
  private:
    bool isnewuser( account_name user ) {
      profiletable profiles(_self, _self);
      
      auto profilesByUser = profiles.get_index<N(getbyuser)>();
      auto p = profilesByUser.find(user);

      return p == profilesByUser.end();
    }

    /// @abi table
    struct profiles {
      uint64_t      id;  // primary key
      account_name  user;      // account name for the user
      std::string   uri;      // the note message
      std::string   hash;
      uint64_t      timestamp; // the store the last update block time

      // primary key
      auto primary_key() const { return id; }
      // secondary key: user
      account_name get_by_user() const { return user; }

      EOSLIB_SERIALIZE( profiles, ( id )( user )( uri )( hash ) ( timestamp ) )
    };

    // create a multi-index table and support secondary key
    typedef eosio::multi_index< N(profiles), profiles,
      indexed_by< N(getbyuser), const_mem_fun<profiles, account_name, &profiles::get_by_user> >
      > profiletable;

  public:
    using contract::contract;

    /// @abi action
    void erase( account_name user) {
      require_auth( user );
      profiletable profiles(_self, _self); // code, scope
      auto profilesByUser = profiles.get_index<N(getbyuser)>();
      auto p = profilesByUser.find(user);
      eosio_assert(p != profilesByUser.end(), "No profile");
      
      profiles.erase(*p);      
    }

    /// @abi action
    void update( account_name user, std::string& uri, std::string& hash ) {      
      require_auth( user );

      profiletable profiles(_self, _self); // code, scope
      
      // create new / update profile depends whether the user account exist or not
      if (isnewuser(user)) {
        print("isnewuser");
        // insert object
        profiles.emplace( _self, [&]( auto& row ) {
          row.id          = profiles.available_primary_key();
          row.user        = user;
          row.uri         = uri;
          row.hash        = hash;
          row.timestamp   = now();
        });
      } else {
        print("isolduser");
        // get object by secordary key
        auto profilesByUser = profiles.get_index<N(getbyuser)>();
        auto &profile = profilesByUser.get(user);
        // update object
        profiles.modify( profile, _self, [&]( auto& address ) {
          address.uri         = uri;
          address.hash        = hash;
          address.timestamp   = now();
        });
      }
    }

};

// specify the contract name, and export a public action: update
EOSIO_ABI( profile, (update)(erase) )
