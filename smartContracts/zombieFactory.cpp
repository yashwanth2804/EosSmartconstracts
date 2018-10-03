//
// Created by Ludvig Kratz on 2018-01-14.
//

#include "zombieFactory.hpp"

namespace ZOMBIES {
    using namespace zombieFactory;

    /*
     * Solidity ses keccak256 to generate random dna
     */
    uint64_t generate_random_dna(eosio::string str) {
        //TODO: Add hash function of str
        uint64_t rand = 8356281049284737;
        return rand;
    }

    void apply_create_random_zombie(const create& c) {

        // Check if creator has an account in the ZombieToOwner table, otherwise create one
        zombie_owner zombie_to_owner;
        bool account_exists = Owners::get(c.owner, zombie_to_owner, N(zombiefac));
        if (account_exists == false) {
            // Create account with zero zombies
            zombie_owner zombie_to_owner(c.owner);
            Owners::store(zombie_to_owner, c.owner);
        }
        assert(zombie_to_owner.counter <= 10, "Maximum of 10 zombies per account!");

        // Generate a random zombie and store it in database
        uint64_t randDna = generate_random_dna(c.name);
        zombie zombie_to_create(c.owner, c.name, randDna);
        Zombies::store(zombie_to_create, c.owner);

        // Update the owner table information
        zombie_to_owner.zombies[zombie_to_owner.counter + 1] = zombie_to_create.dna;
        zombie_to_owner.counter++;
        Owners::update(zombie_to_owner, zombie_to_owner.owner);

    }
} //namespace ZOMBIES

using namespace ZOMBIES;

/**
 *  The init() and apply() methods must have C calling convention so that the blockchain can lookup and
 *  call these methods.
 */
extern "C" {

    /**
     *  This method is called once when the contract is published or updated.
     */
    void init()  {
        eosio::print( "Init!\n" );
    }

    /// The apply method implements the dispatch of events to this contract
    void apply( uint64_t code, uint64_t action_name ) {
        if (code == N(zombiefac)) {
            if (action_name == N(create)){
                ZOMBIES::apply_create_random_zombie(current_message<ZOMBIES::create>());
            }
        }
    }

} // extern "C"

