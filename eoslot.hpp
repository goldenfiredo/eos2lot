#pragma once

#include <eosiolib/asset.hpp>
#include <eosiolib/eosio.hpp>

#include <string>

namespace eosiosystem {
   class system_contract;
}

namespace eosio
{
    using std::string;

    class eoslot : public eosio::contract 
    {
        public:
            eoslot(account_name self) : contract(self) { init_params(); }
            
            void create( account_name issuer, asset maximum_supply);

            void issue( account_name to, asset quantity, string memo );

            void transfer( account_name from, account_name to, asset quantity, string memo );

            void refund(account_name from, account_name to, asset quantity, std::string typ, std::string period, std::string memo) ;

            void init(std::string typ, std::string period, uint8_t stop, std::string memo, uint64_t start_index, uint64_t end_index, uint32_t total_hit, uint32_t total_amount);

        private:
            struct account {
                asset    balance;

                uint64_t primary_key()const { return balance.symbol.name(); }
            };

            struct currency_stats {
                asset          supply;
                asset          max_supply;
                account_name   issuer;

                uint64_t primary_key()const { return supply.symbol.name(); }
            };

            typedef eosio::multi_index<N(accounts), account> accounts;
            typedef eosio::multi_index<N(stat), currency_stats> stats;

            struct issue1
            {
                account_name to;
                asset quantity;
                uint64_t last_issue_time;
                std::string memo;
                
                uint64_t primary_key() const { return to; }

                EOSLIB_SERIALIZE(issue1, (to)(quantity)(last_issue_time)(memo))
            };

            typedef multi_index<N(issue), issue1> issues;

            struct hit
            {
                uint64_t id; 
                account_name from;
                asset quantity;
                uint64_t now;
                std::string memo;
                std::string period;
                std::string typ;

                uint64_t primary_key() const { return id; }

                EOSLIB_SERIALIZE(hit, (id)(from)(quantity)(now)(memo)(period)(typ))
            };

            typedef multi_index<N(hit), hit> hits;
            typedef multi_index<N(hit3d), hit> hits_3d;

            struct refund1
            {
                uint64_t id;
                account_name to;
                asset quantity;
                asset quantity_fee;
                asset quantity_faucet;
                uint64_t now;
                std::string memo;
                std::string period;
                std::string typ;

                uint64_t primary_key() const { return id; }

                EOSLIB_SERIALIZE(refund1, (id)(to)(quantity)(quantity_fee)(quantity_faucet)(now)(memo)(period)(typ))
            };

            typedef multi_index<N(refund1), refund1> refunds;

            struct config
            {
                uint64_t id;
                std::string typ;
                std::string period;
                uint8_t stop;
                std::string memo;
                uint64_t start_index;
                uint64_t end_index;
                uint32_t total_hit;
                uint32_t total_amount;

                uint64_t primary_key() const { return id; }

                EOSLIB_SERIALIZE(config, (id)(typ)(period)(stop)(memo)(start_index)(end_index)(total_hit)(total_amount))
            };

            typedef multi_index<N(config2), config> configs;

            void sub_transfer(account_name from, account_name to, asset quantity, std::string memo);
            void sub_balance( account_name owner, asset value );
            void add_balance( account_name owner, asset value, account_name ram_payer );

            void init_params();
            uint16_t ex_rate = 1000;
            uint16_t per_hit = 100;
            uint8_t max_hit = 50;
            uint8_t deduction = 10;
            
            account_name main_account = N(lotttttttttt);

        public:
            struct transfer_args {
                account_name  from;
                account_name  to;
                asset         quantity;
                string        memo;
            };
    };

    
}


