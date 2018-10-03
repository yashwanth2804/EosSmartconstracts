#include "dtrades.hpp"


void dtrades::listprod(name seller, string metadata, name escrow, asset price) {
  require_auth(seller);
  require_recipient(seller);
  require_recipient(escrow);

  eosio_assert( price.is_valid(), "invalid price" );
  eosio_assert( price.amount > 0, "must specify positive price" );
  eosio_assert( price.symbol == S(4,EOS), "only EOS is accepted" );

  products.emplace( _self, [&]( auto& p ) {
    p.id = products.available_primary_key();
    p.seller = seller;
    p.escrow = escrow;
    p.metadata = metadata;
    p.price = price;
  });
}

void dtrades::editprod(uint64_t id, string metadata, name escrow, asset price) {
  auto product = products.get(id, "invalid product id");

  require_auth(product.seller);
  require_recipient(escrow);

  eosio_assert( price.is_valid(), "invalid price" );
  eosio_assert( price.amount > 0, "must specify positive price" );
  eosio_assert( price.symbol == S(4,EOS), "only EOS is accepted" );

  products.modify( product, _self, [&]( auto& p ) {
    p.escrow = escrow;
    p.metadata = metadata;
    p.price = price;
  });
}

void dtrades::purchase(name buyer, uint64_t product_id, uint64_t quantity, string shipping) {
  require_auth(buyer);
  auto product = products.get(product_id, "invalid product id");

  eosio_assert(quantity > 0, "must purchase positive quantities");
  uint64_t cost = quantity * product.price.amount;
  auto total_price = asset(cost,S(4,EOS));

  orders.emplace( _self, [&]( auto& p ) {
    p.id = orders.available_primary_key();
    p.product_id = product.id;
    p.quantity = quantity;
    p.total_price = total_price;
    p.seller = product.seller;
    p.escrow = product.escrow;
    p.shipping = shipping;
    p.buyer = buyer;
    p.status = "Paid";
  });

  //don't know why this doesn't work
  // action(
  //   permission_level{ buyer, N(active) },
  //   N(eosio.token), N(transfer),
  //   std::make_tuple(buyer, N(dtradesdapp1), asset(cost,S(4,EOS)), "Purchase from DTRADES")
  // ).send();

  INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {buyer,N(active)},
  { buyer, _self, total_price, "Purchase from DTRADES" } );
}

void dtrades::tracking(uint64_t order_id, string details) {
  const auto& order = orders.get(order_id, "invalid order id");
  require_auth(order.seller);
  eosio_assert(order.shipping != "", "no shipping details");
  orders.modify( order, _self, [&]( auto& o ) {
    o.tracking = details;
    o.status = "Shipped";
  });
}

// void dtrades::shipping(uint64_t order_id, string details) {
//   auto order = orders.get(order_id, "invalid order id");
//   require_auth(orders.buyer);
//   orders.modify( order, _self, [&]( auto& o ) {
//     o.shipping = details;
//     o.status = "Ready to ship";
//   }
// }

void dtrades::received(uint64_t order_id) {
  const auto& order = orders.get(order_id, "invalid order id");
  require_auth(order.buyer);
  orders.modify( order, _self, [&]( auto& o ) {
    o.status = "Received";
  });

  uint64_t fee = order.total_price.amount / 20;
  uint64_t payment = fee * 19;

  INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {_self, N(active)},
  { _self, N(dtradesfunds), asset(fee,S(4,EOS)), "Fees from DTRADES" } );

  INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {_self, N(active)},
  { _self, order.seller, asset(payment,S(4,EOS)), "Purchase from DTRADES" } );
}

void dtrades::apprbuyer(uint64_t order_id) {
  const auto& order = orders.get(order_id, "invalid order id");
  require_auth(order.escrow);
  orders.modify( order, _self, [&]( auto& o ) {
    o.status = "Dispute returned to Buyer";
  });

  uint64_t fee = order.total_price.amount / 20;
  uint64_t payment = fee * 19;

  INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {_self, N(active)},
  { _self, N(dtradesfunds), asset(fee,S(4,EOS)), "Fees from DTRADES" } );

  INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {_self, N(active)},
  { _self, order.buyer, asset(payment,S(4,EOS)), "Disputed Purchase from DTRADES" } );
}

void dtrades::apprseller(uint64_t order_id) {
  const auto& order = orders.get(order_id, "invalid order id");
  require_auth(order.escrow);
  orders.modify( order, _self, [&]( auto& o ) {
    o.status = "Dispute paid to Seller";
  });

  uint64_t fee = order.total_price.amount / 20;
  uint64_t payment = fee * 19;

  INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {_self, N(active)},
  { _self, N(dtradesfunds), asset(fee,S(4,EOS)), "Fees from DTRADES" } );

  INLINE_ACTION_SENDER(eosio::token, transfer)( N(eosio.token), {_self, N(active)},
  { _self, order.seller, asset(payment,S(4,EOS)), "Disputed Purchase from DTRADES" } );
}

void dtrades::delorder(uint64_t order_id) {
  require_auth(_self);

  const auto& order = orders.get(order_id, "invalid order id");
  orders.erase(order);
}


EOSIO_ABI(dtrades,
  (listprod)(editprod)(purchase)(tracking)(received)(apprbuyer)(apprseller)(delorder)
)
