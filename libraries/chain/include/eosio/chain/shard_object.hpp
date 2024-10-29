#pragma once
#include <eosio/chain/authority.hpp>
#include <eosio/chain/code_object.hpp>
#include <eosio/chain/block_timestamp.hpp>
#include <eosio/chain/abi_def.hpp>
#include <eosio/chain/transaction.hpp>

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include "multi_index_includes.hpp"

namespace eosio { namespace chain {

   class shard_object : public chainbase::object<shard_object_type, shard_object> {
      OBJECT_CTOR(shard_object)

      id_type                 id;
      shard_name              name;               ///< name should not be changed within a chainbase modifier lambda
      uint32_t                version        = 0; ///< sequentially incrementing version number
      shard_type_enum         shard_type     = shard_type_enum(eosio::chain::shard_type::normal);
      account_name            owner;
      bool                    enabled        = false;
      uint8_t                 opts           = 0; ///< options
      block_timestamp_type    created_time;
      block_timestamp_type    updated_time;

      shard_object& operator=(const shard_object& a) {
         id             = a.id;
         name           = a.name;
         version        = a.version;
         shard_type     = a.shard_type;
         owner          = a.owner;
         enabled        = a.enabled;
         created_time   = a.created_time;
         updated_time   = a.updated_time;
         return *this;
      }
   };
   using shard_id_type = shard_object::id_type;


   // class privacy_shard_object : public chainbase::object<privacy_shard_object_type, shard_object> {
   //    OBJECT_CTOR(privacy_shard_object)

   //    id_type              id;
   //    name                 permmited_account;
   //    shard_name           name;
   //    block_timestamp_type created_time;
   // };

   struct by_name;
   struct by_updated_time;

   using shard_index = chainbase::shared_multi_index_container<
      shard_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<shard_object, shard_object::id_type, &shard_object::id>>,
         ordered_unique<tag<by_name>, member<shard_object, shard_name, &shard_object::name>>,
         ordered_unique<tag<by_updated_time>,
            composite_key< shard_object,
               BOOST_MULTI_INDEX_MEMBER( shard_object, block_timestamp_type, updated_time ),
               BOOST_MULTI_INDEX_MEMBER( shard_object, shard_object::id_type, id)
            >
         >
      >
   >;

   class shard_change_object : public chainbase::object<shard_change_object_type, shard_change_object>
   {
      OBJECT_CTOR(shard_change_object);

      enum class change_type : uint32_t {
         create = 0,
         modify = 1
      };

      id_type                 id;
      shard_name              name;               ///< name should not be changed within a chainbase modifier lambda
      uint32_t                version        = 0; ///< sequentially incrementing version number
      shard_type_enum         shard_type     = shard_type_enum(eosio::chain::shard_type::normal);
      account_name            owner;
      bool                    enabled        = false;
      uint8_t                 opts           = 0; ///< options
      block_num_type          block_num;
   };

   struct by_name;
   struct by_block_num;
   using shard_change_index = chainbase::shared_multi_index_container<
      shard_change_object,
      indexed_by<
         ordered_unique<tag<by_id>, member<shard_change_object, shard_change_object::id_type, &shard_change_object::id>>,
         ordered_unique<tag<by_name>, member<shard_change_object, shard_name, &shard_change_object::name>>
      >
   >;

   struct registered_shard {
      shard_name              name;          //< name should not be changed within a chainbase modifier lambda
      shard_type_enum         shard_type     = shard_type_enum(eosio::chain::shard_type::normal);
      account_name            owner;
      bool                    enabled        = false;
      uint8_t                 opts           = 0; ///< options

      template<typename Object>
      bool has_changed(const Object& obj) const {
         return   owner   != obj.owner
               || enabled != obj.enabled
               || opts     != obj.opts;
      }
   };

   using registered_shard_var = std::variant<registered_shard>;

   struct register_shard_result {
      enum class error_type: int64_t {
         no_change               = -1,
         pending_reg_existed     = -2
      };
   };

} } // eosio::chain

CHAINBASE_SET_INDEX_TYPE(eosio::chain::shard_object, eosio::chain::shard_index)
CHAINBASE_SET_INDEX_TYPE(eosio::chain::shard_change_object, eosio::chain::shard_change_index)

FC_REFLECT(eosio::chain::shard_object, (name)(version)(shard_type)(owner)(enabled)(opts)(created_time)(updated_time))
FC_REFLECT(eosio::chain::shard_change_object, (name)(version)(shard_type)(owner)(enabled)(opts)(block_num))
FC_REFLECT(eosio::chain::registered_shard, (name)(shard_type)(owner)(enabled)(opts))

