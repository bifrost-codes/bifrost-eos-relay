/**
 *  @file
 *  @copyright defined in eos/LICENSE.txt
 */
#pragma once

#include <eosio/chain/wast_to_wasm.hpp>

#include "macro_support.hpp"

/// Some helpful macros to reduce boilerplate when making testcases
/// @{

/**
 * @brief Create/Open a testing_blockchain, optionally with an ID
 *
 * Creates and opens a testing_blockchain with the first argument as its name, and, if present, the second argument as
 * its ID. The ID should be provided without quotes.
 *
 * Example:
 * @code{.cpp}
 * // Create testing_blockchain chain1
 * Make_Blockchain(chain1)
 *
 * // The above creates the following objects:
 * chainbase::database chain1_db;
 * block_log chain1_log;
 * fork_database chain1_fdb;
 * native_contract::native_contract_chain_initializer chain1_initializer;
 * testing_blockchain chain1;
 * @endcode
 */
#define Make_Blockchain(...) BOOST_PP_OVERLOAD(MKCHAIN, __VA_ARGS__)(__VA_ARGS__)
/**
 * @brief Similar to @ref Make_Blockchain, but works with several chains at once
 *
 * Creates and opens several testing_blockchains
 *
 * Example:
 * @code{.cpp}
 * // Create testing_blockchains chain1 and chain2, with chain2 having ID "id2"
 * Make_Blockchains((chain1)(chain2, id2))
 * @endcode
 */
#define Make_Blockchains(...) BOOST_PP_SEQ_FOR_EACH(MKCHAINS_MACRO, _, __VA_ARGS__)

/**
 * @brief Make_Network is a shorthand way to create a testing_network and connect some testing_blockchains to it.
 *
 * Example usage:
 * @code{.cpp}
 * // Create and open testing_blockchains named alice, bob, and charlie
 * MKDBS((alice)(bob)(charlie))
 * // Create a testing_network named net and connect alice and bob to it
 * Make_Network(net, (alice)(bob))
 *
 * // Connect charlie to net, then disconnect alice
 * net.connect_blockchain(charlie);
 * net.disconnect_blockchain(alice);
 *
 * // Create a testing_network named net2 with no blockchains connected
 * Make_Network(net2)
 * @endcode
 */
#define Make_Network(...) BOOST_PP_OVERLOAD(MKNET, __VA_ARGS__)(__VA_ARGS__)

/**
 * @brief Make_Key is a shorthand way to create a keypair
 *
 * @code{.cpp}
 * // This line:
 * Make_Key(a_key)
 * // ...defines these objects:
 * private_key_type a_key_private_key;
 * public_key a_key_public_key;
 * // The private key is generated off of the sha256 hash of "a_key_private_key", so it should be unique from all
 * // other keys created with Make_Key in the same scope.
 * @endcode
 */
#ifdef HAVE_DATABASE_FIXTURE
#define Make_Key(name) auto name ## _private_key = private_key_type::regenerate(fc::digest(#name "_private_key")); \
   store_private_key(name ## _private_key); \
   public_key name ## _public_key = name ## _private_key.get_public_key(); \
   BOOST_TEST_CHECKPOINT("Created key " #name "_public_key");
#else
#define Make_Key(name) auto name ## _private_key = private_key_type::regenerate(fc::digest(#name "_private_key")); \
   public_key name ## _public_key = name ## _private_key.get_public_key(); \
   BOOST_TEST_CHECKPOINT("Created key " #name "_public_key");
#endif

/**
 * @brief Key_Authority is a shorthand way to create an inline Authority based on a key
 *
 * Invoke Key_Authority passing the name of a public key in the current scope, and Key_Authority will resolve inline to
 * an authority which can be satisfied by a signature generated by the corresponding private key.
 */
#define Key_Authority(pubkey) (authority{1, {{pubkey, 1}}, {}})
/**
 * @brief Account_Authority is a shorthand way to create an inline Authority based on an account
 *
 * Invoke Account_Authority passing the name of an account, and Account_Authority will resolve inline to an authority
 * which can be satisfied by the provided account's active authority.
 */
#define Account_Authority(account) (authority{1, {}, {{{#account, "active"}, 1}}})
/**
 * @brief Complex_Authority is a shorthand way to create an arbitrary inline @ref Authority
 *
 * Invoke Complex_Authority passing the weight threshold necessary to satisfy the authority, a bubble list of keys and
 * weights, and a bubble list of accounts and weights.
 *
 * Key bubbles are structured as ((key_name, key_weight))
 * Account bubbles are structured as (("account_name", "account_authority", weight))
 *
 * Example:
 * @code{.cpp}
 * // Create an authority which can be satisfied with a master key, or with any three of:
 * // - key_1
 * // - key_2
 * // - key_3
 * // - Account alice's "test_multisig" authority
 * // - Account bob's "test_multisig" authority
 * Make_Key(master_key)
 * Make_Key(key_1)
 * Make_Key(key_2)
 * Make_Key(key_3)
 * auto auth = Complex_Authority(5, ((master_key, 5))((key_1, 2))((key_2, 2))((key_3, 2)),
 *                               (("alice", "test_multisig", 2))(("bob", "test_multisig", 2));
 * @endcode
 */
#define Complex_Authority(THRESHOLD, KEY_BUBBLES, ACCOUNT_BUBBLES) \
   [&]{ \
      authority x; \
      x.threshold = THRESHOLD; \
      BOOST_PP_SEQ_FOR_EACH(Complex_Authority_macro_Key, x, KEY_BUBBLES) \
      BOOST_PP_SEQ_FOR_EACH(Complex_Authority_macro_Account, x, ACCOUNT_BUBBLES) \
      return x; \
   }()

/**
 * @brief Make_Account is a shorthand way to create an account
 *
 * Use Make_Account to create an account, including keys. The changes will be applied via a transaction applied to the
 * provided blockchain object. The changes will not be incorporated into a block; they will be left in the pending
 * state.
 *
 * Unless overridden, new accounts are created with a balance of asset(100)
 *
 * Example:
 * @code{.cpp}
 * Make_Account(chain, joe)
 * // ... creates these objects:
 * private_key_type joe_private_key;
 * public_key joe_public_key;
 * // ...and also registers the account joe with owner and active authorities satisfied by these keys, created by
 * // init0, with init0's active authority as joe's recovery authority, and initially endowed with asset(100)
 * @endcode
 *
 * You may specify a third argument for the creating account:
 * @code{.cpp}
 * // Same as MKACCT(chain, joe) except that sam will create joe's account instead of init0
 * Make_Account(chain, joe, sam)
 * @endcode
 *
 * You may specify a fourth argument for the amount to transfer in account creation:
 * @code{.cpp}
 * // Same as MKACCT(chain, joe, sam) except that sam will send joe asset(100) during creation
 * Make_Account(chain, joe, sam, asset(100))
 * @endcode
 *
 * You may specify a fifth argument, which will be used as the owner authority (must be an Authority, NOT a key!).
 *
 * You may specify a sixth argument, which will be used as the active authority. If six or more arguments are provided,
 * the default keypair will NOT be created or put into scope.
 *
 * You may specify a seventh argument, which will be used as the recovery authority.
 */
#define Make_Account(...) BOOST_PP_OVERLOAD(MKACCT, __VA_ARGS__)(__VA_ARGS__)

/**
 * @brief Shorthand way to set the code for an account
 *
 * @code{.cpp}
 * char* wast = //...
 * Set_Code(chain, codeacct, wast);
 * @endcode
 */
#define Set_Code(...) BOOST_PP_OVERLOAD(SETCODE, __VA_ARGS__)(__VA_ARGS__)

/**
 * @brief Shorthand way to create or update named authority on an account
 *
 * @code{.cpp}
 * // Add a new authority named "money" to account "alice" as a child of her active authority
 * authority newAuth = //...
 * Set_Authority(chain, alice, "money", "active", newAuth);
 * @endcode
 */
#define Set_Authority(...) BOOST_PP_OVERLOAD(SETAUTH, __VA_ARGS__)(__VA_ARGS__)
/**
 * @brief Shorthand way to delete named authority from an account
 *
 * @code{.cpp}
 * // Delete authority named "money" from account "alice"
 * Delete_Authority(chain, alice, "money");
 * @endcode
 */
#define Delete_Authority(...) BOOST_PP_OVERLOAD(DELAUTH, __VA_ARGS__)(__VA_ARGS__)
/**
 * @brief Shorthand way to link named authority with a contract/message type
 *
 * @code{.cpp}
 * // Link alice's "money" authority with eosio::transfer
 * Link_Authority(chain, alice, "money", eos, "transfer");
 * // Set alice's "native" authority as default for eos contract
 * Link_Authority(chain, alice, "money", eos);
 * @endcode
 */
#define Link_Authority(...) BOOST_PP_OVERLOAD(LINKAUTH, __VA_ARGS__)(__VA_ARGS__)
/**
 * @brief Shorthand way to unlink named authority from a contract/message type
 *
 * @code{.cpp}
 * // Unlink alice's authority for eosio::transfer
 * Unlink_Authority(chain, alice, eos, "transfer");
 * // Unset alice's default authority for eos contract
 * Unlink_Authority(chain, alice, eos);
 * @endcode
 */
#define Unlink_Authority(...) BOOST_PP_OVERLOAD(UNLINKAUTH, __VA_ARGS__)(__VA_ARGS__)

/**
 * @brief Shorthand way to transfer funds
 *
 * Use Transfer_Asset to send funds from one account to another:
 * @code{.cpp}
 * // Send 10 EOS from alice to bob
 * Transfer_Asset(chain, alice, bob, asset(10));
 *
 * // Send 10 EOS from alice to bob with memo "Thanks for all the fish!"
 * Transfer_Asset(chain, alice, bob, asset(10), "Thanks for all the fish!");
 * @endcode
 *
 * The changes will be applied via a transaction applied to the provided blockchain object. The changes will not be
 * incorporated into a block; they will be left in the pending state.
 */
#define Transfer_Asset(...) BOOST_PP_OVERLOAD(XFER, __VA_ARGS__)(__VA_ARGS__)

/**
 * @brief Shorthand way to convert liquid funds to staked funds
 *
 * Use Stake_Asset to stake liquid funds:
 * @code{.cpp}
 * // Convert 10 of bob's EOS from liquid to staked
 * Stake_Asset(chain, bob, asset(10).amount);
 *
 * // Stake and transfer 10 EOS from alice to bob (alice pays liquid EOS and bob receives stake)
 * Stake_Asset(chain, alice, bob, asset(10).amount);
 * @endcode
 */
#define Stake_Asset(...) BOOST_PP_OVERLOAD(STAKE, __VA_ARGS__)(__VA_ARGS__)

/**
 * @brief Shorthand way to begin conversion from staked funds to liquid funds
 *
 * Use Unstake_Asset to begin unstaking funds:
 * @code{.cpp}
 * // Begin unstaking 10 of bob's EOS
 * Unstake_Asset(chain, bob, asset(10).amount);
 * @endcode
 *
 * This can also be used to cancel an unstaking in progress, by passing asset(0) as the amount.
 */
#define Begin_Unstake_Asset(...) BOOST_PP_OVERLOAD(BEGIN_UNSTAKE, __VA_ARGS__)(__VA_ARGS__)

/**
 * @brief Shorthand way to claim unstaked EOS as liquid
 *
 * Use Finish_Unstake_Asset to liquidate unstaked funds:
 * @code{.cpp}
 * // Reclaim as liquid 10 of bob's unstaked EOS
 * Unstake_Asset(chain, bob, asset(10).amount);
 * @endcode
 */
#define Finish_Unstake_Asset(...) BOOST_PP_OVERLOAD(FINISH_UNSTAKE, __VA_ARGS__)(__VA_ARGS__)


/**
 * @brief Shorthand way to set voting proxy
 *
 * Use Set_Proxy to set what account a stakeholding account proxies its voting power to
 * @code{.cpp}
 * // Proxy sam's votes to bob
 * Set_Proxy(chain, sam, bob);
 *
 * // Unproxy sam's votes
 * Set_Proxy(chain, sam, sam);
 * @endcode
 */
#define Set_Proxy(chain, stakeholder, proxy) \
{ \
   eosio::chain::signed_transaction trx; \
   if (std::string(#stakeholder) != std::string(#proxy)) \
      transaction_emplace_message(trx, config::eos_contract_name, \
                         vector<types::account_permission>{ {#stakeholder,"active"} }, "setproxy", types::setproxy{#stakeholder, #proxy}); \
   else \
      transaction_emplace_message(trx, config::eos_contract_name, \
                         vector<types::account_permission>{ {#stakeholder,"active"} }, "setproxy", types::setproxy{#stakeholder, #proxy}); \
   trx.expiration = chain.head_block_time() + 100; \
   transaction_set_reference_block(trx, chain.head_block_id()); \
   chain.push_transaction(trx); \
}

/**
 * @brief Shorthand way to create a block producer
 *
 * Use Make_Producer to create a block producer:
 * @code{.cpp}
 * // Create a block producer belonging to joe using signing_key as the block signing key and config as the producer's
 * // vote for a @ref BlockchainConfiguration:
 * Make_Producer(chain, joe, signing_key, config);
 *
 * // Create a block producer belonging to joe using signing_key as the block signing key:
 * Make_Producer(chain, joe, signing_key);
 *
 * // Create a block producer belonging to joe, using a new key as the block signing key:
 * Make_Producer(chain, joe);
 * // ... creates the objects:
 * private_key_type joe_producer_private_key;
 * public_key joe_producer_public_key;
 * @endcode
 */
#define Make_Producer(...) BOOST_PP_OVERLOAD(MKPDCR, __VA_ARGS__)(__VA_ARGS__)

/**
 * @brief Shorthand way to set approval of a block producer
 *
 * Use Approve_Producer to change an account's approval of a block producer:
 * @code{.cpp}
 * // Set joe's approval for pete's block producer to Approve
 * Approve_Producer(chain, joe, pete, true);
 * // Set joe's approval for pete's block producer to Disapprove
 * Approve_Producer(chain, joe, pete, false);
 * @endcode
 */
#define Approve_Producer(...) BOOST_PP_OVERLOAD(APPDCR, __VA_ARGS__)(__VA_ARGS__)

/**
 * @brief Shorthand way to update a block producer
 *
 * @note Unlike with the Make_* macros, the Update_* macros take an expression as the owner/name field, so be sure to
 * wrap names like this in quotes. You may also pass a normal C++ expression to be evaulated here instead. The reason
 * for this discrepancy is that the Make_* macros add identifiers to the current scope based on the owner/name field;
 * moreover, which can't be done with C++ expressions; however, the Update_* macros do not add anything to the scope,
 * and it's more likely that these will be used in a loop or other context where it is inconvenient to know the
 * owner/name at compile time.
 *
 * Use Update_Producer to update a block producer:
 * @code{.cpp}
 * // Update a block producer belonging to joe using signing_key as the new block signing key, and config as the
 * // producer's new vote for a @ref BlockchainConfiguration:
 * Update_Producer(chain, "joe", signing_key, config)
 *
 * // Update a block producer belonging to joe using signing_key as the new block signing key:
 * Update_Producer(chain, "joe", signing_key)
 * @endcode
 */
#define Update_Producer(...) BOOST_PP_OVERLOAD(UPPDCR, __VA_ARGS__)(__VA_ARGS__)

/// @}
