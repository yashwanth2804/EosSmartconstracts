#include <eosiolib/eosio.hpp>
#include <eosiolib/print.hpp>
using namespace eosio;

class fuzzy : public eosio::contract { 
  private:
  public:
    using contract::contract;

    /// @abi action
    void publishdata( account_name user, std::string& hash, std::string& location) {      
      require_auth( user );
    }
};
EOSIO_ABI( fuzzy, (publishdata) )
