#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <string>
#include "eosio.token.hpp"

using namespace eosio;
using std::string;
using eosio::name;

class dtrades : public contract {

public:
  dtrades(account_name self)
  :contract(self),
  products(_self, _self),
  orders(_self, _self){}

  [[eosio::action]] void listprod(name seller, string metadata, name escrow, asset price);
  [[eosio::action]] void editprod(uint64_t id, string metadata, name escrow, asset price);
  [[eosio::action]] void purchase(name buyer, uint64_t product_id, uint64_t quantity, string shipping);
  [[eosio::action]] void tracking(uint64_t order_id, string details);
  //[[eosio::action]] void shipping(uint64_t order_id, string details);
  [[eosio::action]] void received(uint64_t order_id);
  [[eosio::action]] void apprbuyer(uint64_t order_id);
  [[eosio::action]] void apprseller(uint64_t order_id);
  [[eosio::action]] void delorder(uint64_t order_id);

private:
  // Helper Structs
  struct [[eosio::table]] st_products {
    uint64_t id;
    name     seller;
    name     escrow;
    string   metadata;
    asset    price;
    uint64_t primary_key() const { return id; }
  };

  typedef multi_index<N(products), st_products> tb_products;
  tb_products products;

  struct [[eosio::table]] st_orders {
    uint64_t id;
    uint64_t product_id;
    uint64_t quantity;
    asset    total_price;
    name     seller;
    name     buyer;
    name     escrow;
    string   shipping;
    string   tracking;
    string   status;
    uint64_t primary_key() const { return id; }
  };

  typedef multi_index<N(orders), st_orders> tb_orders;
  tb_orders orders;
};
