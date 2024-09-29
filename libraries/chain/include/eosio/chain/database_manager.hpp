#pragma once

#include <chainbase/chainbase.hpp>
#include <eosio/chain/types.hpp>
namespace eosio{ namespace chain {

   /**
    *  database manager
    */
   class database_manager
   {
      public:
         typedef chainbase::database               database;
         typedef chainbase::pinnable_mapped_file   pinnable_mapped_file;
         typedef boost::filesystem::path           path;
         typedef database::open_flags              open_flags;

         using database_index_row_count_multiset = std::multiset<std::pair<unsigned, std::string>>;

         database_manager(const path& dir, open_flags write = open_flags::read_only,
                  uint64_t shared_file_size = 0, uint64_t main_file_size = 0,
                  bool allow_dirty = false,
                  pinnable_mapped_file::map_mode = pinnable_mapped_file::map_mode::mapped);
         database_manager(database_manager&&) = default;
         database_manager& operator=(database_manager&&) = default;
         bool is_read_only() const { return _read_only; }
         void flush();

         const database& shared_db() const { return _shared_db; }
         database& shared_db() { return _shared_db; }

         const database& main_db() const { return _main_db; }
         database& main_db() { return _main_db; }

         const database& shard_db(db_name shard_name)  const;
         database& shard_db( db_name shard_name);

         database* find_shard_db(const shard_name& name);
         const database* find_shard_db(const shard_name& name) const;
         std::map<db_name, database>& shard_dbs() { return _shard_db_map; }

         struct session {
            public:
               session( session&& s ):_db_sessions( std::move(s._db_sessions) ){}
               session( std::vector<std::unique_ptr<database::session>>&& s ):_db_sessions( std::move(s) )
               {
               }

               ~session() {
                  undo();
               }

               void push()
               {
                  for( auto& i : _db_sessions ) i->push();
                  _db_sessions.clear();
               }

               void squash()
               {
                  for( auto& i : _db_sessions ) i->squash();
                  _db_sessions.clear();
               }

               void undo()
               {
                  for( auto& i : _db_sessions ) i->undo();
                  _db_sessions.clear();
               }

            private:
               friend class database_manager;
               session(){}

               std::vector< std::unique_ptr<database::session> > _db_sessions;
         };

         session start_undo_session( bool enabled );

         int64_t revision()const {
            return _main_db.revision();
         }

         int64_t shard_revision( db_name shard_name ) const {
            return _shard_db_map.at(shard_name).revision();
         }
         std::map<db_name, database>& sub_shard_dbs() { return _shard_db_map;}
         void undo();
         void squash();
         void commit( int64_t revision );
         void undo_all();


         void set_revision( uint64_t revision )
         {
             if ( _read_only_mode ) {
                BOOST_THROW_EXCEPTION( std::logic_error( "attempting to set revision in read-only mode" ) );
             }
             _shared_db.set_revision(revision);
             _main_db.set_revision(revision);
            //shards db set_revision
            for ( auto& db : _shard_db_map ) {
               db.second.set_revision( revision );
            }
         }

         database* add_shard_db( const shard_name& name, uint64_t shared_file_size = 0 );

         template<typename MultiIndexType>
         void add_index() {
             _shared_db.add_index<MultiIndexType>();
             _main_db.add_index<MultiIndexType>();
         }

         template<typename IndexSetType>
         void add_indices_to_shard_db() {
            IndexSetType::add_indices(_main_db);
            //add index set to every sub shard db
            for ( auto& db : _shard_db_map ) {
               IndexSetType::add_indices(db.second);
            }
         }

         void set_read_only_mode() {
            _read_only_mode = true;
            _shared_db.set_read_only_mode();
            _main_db.set_read_only_mode();
            // set every shard_db to read only mode
            for ( auto& db : _shard_db_map ) {
               db.second.unset_read_only_mode();
            }
         }

         void unset_read_only_mode() {
             if ( _read_only )
                BOOST_THROW_EXCEPTION( std::logic_error( "attempting to unset read_only_mode while database was opened as read only" ) );
            _read_only_mode = false;
            _shared_db.unset_read_only_mode();
            _main_db.unset_read_only_mode();
            for ( auto& db : _shard_db_map ) {
               db.second.unset_read_only_mode();
            }
         }

      public:
         path                             dir;
         open_flags                       flags;
         bool                             allow_dirty = false;
         pinnable_mapped_file::map_mode   db_map_mode = pinnable_mapped_file::map_mode::mapped;

      private:
         database                         _shared_db;
         database                         _main_db;
         std::map<db_name, database>      _shard_db_map;
         bool                             _read_only = false;

         /**
          * _read_only_mode is dynamic which can be toggled back and for
          * by users, while _read_only is static throughout the lifetime
          * of the database instance. When _read_only_mode is set to true,
          * an exception is thrown when modification attempt is made on
          * chainbase. This ensures state is not modified by mistake when
          * application does not intend to change state.
          */
         bool                             _read_only_mode      = false;
         bool                             _is_saving_catalog   = false;
   };

   struct shard_db_catalog {
      static const uint32_t magic_number;
      static const uint32_t min_supported_version;
      static const uint32_t max_supported_version;

      block_id_type           head_block;
      std::set<shard_name> shards;
      std::string error_msg;

   };

   using shard_db_catalog_ptr = std::shared_ptr<shard_db_catalog>;

   struct sdb_catalog_reader {

      // static void save(database_manager& dbm);
      static shard_db_catalog_ptr read(const fc::path& dir);
   };

   struct sdb_catalog_writer {
      sdb_catalog_writer(database_manager& dbm);

      void write();

      database_manager& dbm;
   };

   using sdb_catalog_writer_ptr = std::shared_ptr<sdb_catalog_writer>;

}}  // namepsace chainbase


// FC_REFLECT(eosio::chain::shard_db_info, (shards))
