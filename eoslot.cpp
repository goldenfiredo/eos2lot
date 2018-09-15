#include "eoslot.hpp"

namespace eosio
{

  void eoslot::create( account_name issuer, asset maximum_supply )
  {
      require_auth( _self );

      auto sym = maximum_supply.symbol;
      eosio_assert( sym.is_valid(), "invalid symbol name" );
      eosio_assert( maximum_supply.is_valid(), "invalid supply");
      eosio_assert( maximum_supply.amount > 0, "max-supply must be positive");

      stats statstable( _self, sym.name() );
      auto existing = statstable.find( sym.name() );
      eosio_assert( existing == statstable.end(), "token with symbol already exists" );

      statstable.emplace( _self, [&]( auto& s ) {
        s.supply.symbol = maximum_supply.symbol;
        s.max_supply    = maximum_supply;
        s.issuer        = issuer;
      });
  }


  void eoslot::issue( account_name to, asset quantity, string memo )
  {
      auto sym = quantity.symbol;
      eosio_assert( sym.is_valid(), "invalid symbol name" );
      eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );

      auto sym_name = sym.name();
      stats statstable( _self, sym_name );
      auto existing = statstable.find( sym_name );
      eosio_assert( existing != statstable.end(), "token with symbol does not exist, create token before issue" );
      const auto& st = *existing;

      require_auth( st.issuer );
      eosio_assert( quantity.is_valid(), "invalid quantity" );
      eosio_assert( quantity.amount > 0, "must issue positive quantity" );

      eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
      eosio_assert( quantity.amount <= st.max_supply.amount - st.supply.amount, "quantity exceeds available supply");

      statstable.modify( st, 0, [&]( auto& s ) {
        s.supply += quantity;
      });

      add_balance( st.issuer, quantity, st.issuer );

      if( to != st.issuer ) {
        SEND_INLINE_ACTION( *this, transfer, {st.issuer,N(active)}, {st.issuer, to, quantity, memo} );
      }
  }

  void eoslot::transfer(account_name from, account_name to, asset quantity, std::string memo)
  {
      if (quantity.symbol == S(4, EOS) && from == N(lottttttteos) && to == main_account) {
        return;
      }

      if (quantity.symbol == S(4, LOT) && (to != main_account || (from == N(lotttttttlot) && to == main_account))) {
        sub_transfer(from, to, quantity, memo);

        return;
      }

      if(quantity.symbol == S(4, EOS) && from != main_account && to == main_account) {
        
        asset quant{quantity.amount * ex_rate, S(4, LOT)};

        // eosio.code permission was required.
        action(
            permission_level{ to, N(active) },
            main_account, N(issue),
            std::make_tuple(from, quant, std::string("thanks"))
            //N(lotttttttttt), N(transfer),
            //std::make_tuple(to, from, quant, std::string("thanks"))
        ).send();

        /*issues issues_table(_self, to);

        auto toitr = issues_table.find( from );
        if ( toitr == issues_table.end() ) 
        {
          issues_table.emplace(_self, [&](auto &s) {
              s.to = from;
              s.quantity = quant;
              s.memo = memo;
              s.last_issue_time = now();
          });
        }
        else
        {
          issues_table.modify( toitr, 0, [&]( auto& a ) {
              a.quantity.amount += quant.amount;
              a.last_issue_time = now();
              eosio_assert( a.quantity.amount >= quant.amount, "overflow detected" );
           });
        }*/

        return;
      }

      if(quantity.symbol == S(4, LOT) && from != main_account && to == main_account) {
        eosio_assert( is_account( from ), "from account does not exist");
        eosio_assert( memo.length() > 0, "bad memo" );

        std::string delim = std::string("-");
        size_t index = memo.find_first_of(delim,0);
        eosio_assert(index!=std::string::npos, "bad type");
        std::string typ = memo.substr(0, index);

        size_t index1 = memo.find_first_of(delim,index + 1);
        eosio_assert(index1!=std::string::npos, "bad period");
        std::string period = memo.substr(index+1, index1 - index - 1);

        std::string hit = memo.substr(index1 + 1);
        eosio_assert( hit.length() > 0, "bad hit" );

        delim = std::string(" ");
        index = 0;
        size_t count = 0;
        while (index!=std::string::npos) {
          index = hit.find_first_of(delim,index+1);
          count++;
        }

        configs config_table(_self, main_account);

        bool found = false;
        bool stop = true;
        auto itor = config_table.begin();
        while ( itor != config_table.end() ) {
          if (itor->typ == typ && itor->period == period) {
            found = true;
            stop = itor->stop;
            break;
          }
          itor++;
        }

        eosio_assert( found, "bad type or period" );

        eosio_assert( !stop, "hit has stopped" );

        eosio_assert((typ == std::string("db") && count == 7) || (typ == std::string("3d") && count == 3), "bad hit number");

        eosio_assert(quantity.amount % (per_hit * 10000) == 0, "bad amount");

        if (typ == std::string("3d")) {
          hits_3d hit_table(_self, to);
          int64_t total = 0;
          auto current = hit_table.begin();
          while ( current != hit_table.end() ) {
            account_name h_from = current->from;
            std::string h_typ = current->typ;
            std::string h_period = current->period;
            if (h_typ == typ && h_period == period && h_from == from)
            {
              total += current->quantity.amount;
            }
            current++;
          }

          eosio_assert(total + quantity.amount <= per_hit * max_hit * 10000, "reach limit");

          sub_transfer(from, to, quantity, memo);

          hit_table.emplace(_self, [&](auto &s) {
              s.id = hit_table.available_primary_key();
              s.from = from;
              s.quantity = quantity;
              s.memo = hit;
              s.now = now();
              s.period = period;
              s.typ = typ;
          });

          return;
        }

        hits hit_table(_self, to);
        int64_t total = 0;
        auto current = hit_table.begin();
        while ( current != hit_table.end() ) {
          account_name h_from = current->from;
          std::string h_typ = current->typ;
          std::string h_period = current->period;
          if (h_typ == typ && h_period == period && h_from == from)
          {
            total += current->quantity.amount;
          }
          current++;
        }

        eosio_assert(total + quantity.amount <= per_hit * max_hit * 10000, "reach limit");

        sub_transfer(from, to, quantity, memo);

        hit_table.emplace(_self, [&](auto &s) {
            s.id = hit_table.available_primary_key();
            s.from = from;
            s.quantity = quantity;
            s.memo = hit;
            s.now = now();
            s.period = period;
            s.typ = typ;
        });

        return;
      }
  }

  void eoslot::refund(account_name from, account_name to, asset quantity, std::string typ, std::string period, std::string memo) 
  {
      require_auth( from );

      if(from == main_account && to != main_account && quantity.symbol == S(4, LOT) ) {
        eosio_assert( is_account( to ), "to account does not exist");
        eosio_assert( typ.length() > 0, "bad type" );
        eosio_assert( period.length() > 0, "bad period" );

        configs config_table(_self, main_account);
        
        bool found = false;
        bool stop = false;
        auto itor = config_table.begin();
        while ( itor != config_table.end() ) {
          if (itor->typ == typ && itor->period == period) {
            found = true;
            stop = itor->stop;
            break;
          }
          itor++;
        }

        eosio_assert( found, "bad type or period" );

        eosio_assert( stop, "hit not stopped" );

        account_name lot_account = N(lotttttttlot);
        
        action(
            permission_level{ from, N(active) },
            main_account, N(transfer),
            std::make_tuple(from, lot_account, quantity, std::string("transfer LOT"))
        ).send();

        
        asset quant{quantity.amount * (100 - deduction) / 100000, S(4, EOS)};

        action(
            permission_level{ from, N(active) },
            N(eosio.token), N(transfer),
            std::make_tuple(from, to, quant, std::string("refund EOS"))
        ).send();

        account_name fee_account = N(lottttttteos);
        
        asset fee_quant{quantity.amount * deduction / 100000, S(4, EOS)};

        action(
            permission_level{ from, N(active) },
            N(eosio.token), N(transfer),
            std::make_tuple(from, fee_account, fee_quant, std::string("transfer eos"))
        ).send();

        refunds refund_table(_self, from);

        refund_table.emplace(_self, [&](auto &s) {
            s.id = refund_table.available_primary_key();
            s.to = to;
            s.quantity = quant;
            s.quantity_fee = fee_quant;
            s.quantity_faucet = quantity;
            s.memo = memo;
            s.now = now();
            s.period = period;
            s.typ = typ;
        });

        return;
      }
  }

  void eoslot::init(std::string typ, std::string period, uint8_t stop, std::string memo, uint64_t start_index, uint64_t end_index, uint32_t total_hit, uint32_t total_amount) 
  {
      require_auth( main_account );

      configs config_table(_self, main_account);
        
      bool found = false;
      auto current = config_table.begin();
      while ( current != config_table.end() ) {
        if (current->typ == typ && current->period == period) {
          found = true;
          break;
        }
        current++;
      }

      if (!found) {
        config_table.emplace(_self, [&](auto &s) {
            s.id = config_table.available_primary_key();
            s.typ = typ;
            s.period = period;
            s.stop = stop;
            s.memo = memo;
            s.start_index = start_index;
            s.end_index = end_index;
            s.total_hit = total_hit;
            s.total_amount = total_amount;
        });

      } 
      else {
        auto toitr = config_table.find( current->id );
        //if ( toitr != config_table.end() ) {
          config_table.modify( toitr, 0, [&]( auto& a ) {
            a.period = period;
            a.stop = stop;
            a.memo = memo;   
            a.start_index = start_index;
            a.end_index = end_index;
            a.total_hit = total_hit;
            a.total_amount = total_amount;   
          });
        //}
      }
  }

  //common
  void eoslot::sub_transfer(account_name from, account_name to, asset quantity, std::string memo) {
    eosio_assert( from != to, "cannot transfer to self" );
    require_auth( from );
    eosio_assert( is_account( to ), "to account does not exist");
    auto sym = quantity.symbol.name();
    stats statstable( _self, sym );
    const auto& st = statstable.get( sym );

    require_recipient( from );
    require_recipient( to );

    eosio_assert( quantity.is_valid(), "invalid quantity" );
    eosio_assert( quantity.amount > 0, "must transfer positive quantity" );
    eosio_assert( quantity.symbol == st.supply.symbol, "symbol precision mismatch" );
    eosio_assert( memo.size() <= 256, "memo has more than 256 bytes" );


    sub_balance( from, quantity );
    add_balance( to, quantity, from );
  }

  //private functions
  void eoslot::init_params() {
      configs config_table(_self, main_account);
      
      std::string init_memo = std::string("");
      auto itor = config_table.begin();
      while ( itor != config_table.end() ) {
        if (itor->typ == std::string("init")) {
          init_memo = itor->memo;
          break;
        }
        itor++;
      }

      std::string delim = std::string("-");

      if (init_memo.length() > 0) 
      {
        size_t index = init_memo.find_first_of(delim,0);
        if (index!=std::string::npos) {
          std::string s_ex_rate = init_memo.substr(0, index);
          ex_rate = std::stoi(s_ex_rate);

          size_t index1 = init_memo.find_first_of(delim,index + 1);
          if (index1!=std::string::npos) {
            std::string s_per_hit = init_memo.substr(index+1, index1-index-1);
            per_hit = std::stoi(s_per_hit);
            size_t index2 = init_memo.find_first_of(delim,index1 + 1);
            if (index2!=std::string::npos) {
              std::string s_max_hit = init_memo.substr(index1+1, index2-index1-1);
              max_hit = std::stoi(s_max_hit);

              deduction = std::stoi(init_memo.substr(index2+1));
            } else {
              max_hit = std::stoi(init_memo.substr(index1+1));
            }
          } else {
            per_hit = std::stoi(init_memo.substr(index+1));
          }
        } else {
          ex_rate = std::stoi(init_memo);
        }
      }
  }

  void eoslot::sub_balance( account_name owner, asset value ) {
    accounts from_acnts( _self, owner );

    const auto& from = from_acnts.get( value.symbol.name(), "no balance object found" );
    eosio_assert( from.balance.amount >= value.amount, "overdrawn balance" );


    if( from.balance.amount == value.amount ) {
        from_acnts.erase( from );
    } else {
        from_acnts.modify( from, owner, [&]( auto& a ) {
            a.balance -= value;
        });
    }
  }

  void eoslot::add_balance( account_name owner, asset value, account_name ram_payer )
  {
    accounts to_acnts( _self, owner );
    auto to = to_acnts.find( value.symbol.name() );
    if( to == to_acnts.end() ) {
        to_acnts.emplace( ram_payer, [&]( auto& a ){
          a.balance = value;
        });
    } else {
        to_acnts.modify( to, 0, [&]( auto& a ) {
          a.balance += value;
        });
    }
  }
}


EOSIO_ABI(eosio::eoslot, (create)(issue)(transfer)(refund)(init))
