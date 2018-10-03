//
// Created by Ludvig Kratz on 2018-01-14.
//

//#include <stdio.h>
//#include <math.h>
#include <eoslib/eos.hpp>
#include <eoslib/db.hpp>
#include <eoslib/string.hpp>


using namespace eosio;
namespace zombieFactory {

    struct zombie {
        zombie() {};
        zombie(account_name owner, eosio::string name, uint64_t dna):owner(owner), name(name), dna(dna) {};
        uint64_t dna; // DNA of zombie will be the key (since this is the first uint64_t in the struct)
        account_name owner; // Owner of the zombie
        eosio::string name;
    };

    struct zombie_owner {
        zombie_owner() {};
        zombie_owner(account_name owner):owner(owner) { counter = 0; };
        account_name owner; //Database table key
        uint8_t max_zombies = 10; // Maximum 10 zombies per account, make vector?
        uint64_t zombies[10]; //List of zombie DNA this user owns, TODO: maybe make it Zombie object
        uint64_t counter; // i.e length of zombies array
    };

    // Defines a action used when calling the smart contract
    struct create {
        account_name owner;
        eosio::string name;
    };

    // Define database table on blockchain
    using Zombies = eosio::table<N(zombiefac), // default scope
                                 N(zombiefac), // Defines account that has write access
                                 N(zombies),       // Name of the table
                                 zombie,           // Structure of table (c++ struct)
                                 uint64_t>;        // Data type of table's key (first uint64_t that's defined in struct
                                                   // will be the key
    // Mapping of a users' zombies
    using Owners = eosio::table<N(zombiefac), N(zombiefac), N(owners), zombie_owner, uint64_t>;

}