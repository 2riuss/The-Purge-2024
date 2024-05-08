#include "Player.hh"

/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME Bernat


struct PLAYER_NAME : public Player {

  /**
   * Factory: returns a new instance of this class.
   * Do not modify this function.
   */
  static Player* factory () {
    return new PLAYER_NAME;
  }

  /**
   * Types and attributes for your player can be defined here.
   */
  int numBarricades = 0;
  vector<Dir> Directions = {Down,Right,Up,Left};

  struct Board_cell {
    int dist;
    vector<int> dirs;
    bool vist;

    Board_cell() {
      dist = -1;
      vist = false;
    }
  };

  struct Board {
    vector<vector<Board_cell>> B;

    Board(int rows, int cols) {
      B = vector<vector<Board_cell>> (rows, vector<Board_cell> (cols));
    }

    Board_cell& operator[](Pos p) {
      return B[p.i][p.j];
    }
  };
  
  int days_to_move(int id, int rd, Cell c) {
    int days_to_start = 0;
    int days_to_break = 0;
    if (c.b_owner != -1 and c.b_owner != me() and is_round_day(rd)) days_to_start = 25-rd%25;
    switch (citizen(id).weapon) {
      case Hammer: 
        days_to_break = c.resistance/hammer_strength_demolish();
        break;
      case Gun:
        days_to_break = c.resistance/gun_strength_demolish();
        break;
      case Bazooka:
        days_to_break = c.resistance/bazooka_strength_demolish();
        break;
      case NoWeapon:
        days_to_break = c.resistance/builder_strength_demolish();
        break;
    }
    return days_to_start + days_to_break + 1;
  }

  double bonus_value(Pos p, double dist, int id) {
    if (cell(p).bonus == Money) return 10./dist;
    double x = ceil((double) citizen(id).life/life_lost_in_attack());
    return 1000./(x*x*x*x*dist);
  }

  int tier(WeaponType w) {
    switch (w) {
      case NoWeapon: return 0;
      case Hammer: return 1;
      case Gun: return 2;
      case Bazooka: return 3;
      default: return -1;
    }
  }

  double weapon_value(Pos p, double dist, int id) {
    if (cell(p).weapon == Bazooka) return 100000/dist;
    else return 10000/dist;
  }

  double citizen_value(Pos p, double dist, int id) {
    double value = 0;
    double myTier = tier(citizen(id).weapon);
    double hisTier = tier(citizen(cell(p).id).weapon);
    double myHelth = ceil((double) citizen(id).life/life_lost_in_attack());
    double hisHelth = ceil((double) citizen(cell(p).id).life/life_lost_in_attack());
    double hisPoints = score(citizen(cell(p).id).player);
    double days2night = num_rounds_per_day()/2 - round()%(num_rounds_per_day()/2);
    double days2day = num_rounds_per_day()/2 - round()%(num_rounds_per_day()/2);
    if (myTier - hisTier < 0 and is_day()) {
      value = -(1./myHelth)*((hisPoints/100.+1000.)/(days2night*(dist/2.)*(dist/2.)));
    }
    else if (myTier - hisTier < 0 and is_night()) {
      if (days2day < dist/2.) value = 0;
      else value = -(1./myHelth)*((hisPoints/100.+1000.)/((dist/2.)*(dist/2.)))*(days2day-dist/2.);
    }
    else if (myTier - hisTier == 0 and is_day()) {
      value = ((myHelth-hisHelth)/7.)*((hisPoints/100.+1000.)/(days2night*(dist/2.)*(dist/2.)));
    }
    else if (myTier - hisTier == 0 and is_night()) {
      if (days2day < dist/2.) value = 0;
      else value = ((myHelth-hisHelth)/7.)*((hisPoints/100.+1000.)/((dist/2.)*(dist/2.)))*(days2day-dist/2.);
    }
    else if (myTier - hisTier > 0 and is_day()) {
      value = (1.-1./(2.*myHelth*myHelth))*((100.*hisTier+hisPoints/100.+1000.)/(days2night*(dist/2.)*(dist/2.)));
    }
    else if (myTier - hisTier > 0 and is_night()) {
      if (days2day < dist/2.) value = 0;
      else value = (1.-1./(2.*myHelth*myHelth))*((100.*hisTier+hisPoints/100.+1000.)/((dist/2.)*(dist/2.)))*(days2day-dist/2.);
    }
    return value;
  }

  void find(int id) {
    Board B(board_rows(), board_cols());
    priority_queue<pair<int,Pos>, vector<pair<int,Pos>>, greater<pair<int,Pos>>> q;
    set<Pos> bonus;
    set<Pos> weapons;
    set<Pos> citizens;

    Pos a = citizen(id).pos;
    B[a].dist = 0;
    B[a].vist = true;
    for (int i = 0; i < 4; ++i) {
      if (pos_ok(a+Directions[i]) and cell(a+Directions[i]).type == Street) {
        int dist = days_to_move(id, round(), cell(a+Directions[i]));
        B[a+Directions[i]].dist = dist;
        B[a+Directions[i]].dirs = vector<int> (4,false);
        B[a+Directions[i]].dirs[i] = true;
        q.push(make_pair(B[a+Directions[i]].dist, a+Directions[i]));
      }
    }

    while(not q.empty() and q.top().first < 10) {
      Pos b = q.top().second;
      q.pop();
      if (not B[b].vist) {
        B[b].vist = true;
        if (cell(b).bonus != NoBonus) bonus.insert(b);
        else if (cell(b).weapon != NoWeapon) weapons.insert(b);
        else if (cell(b).id != -1 and citizen(cell(b).id).player != me()) citizens.insert(b);
        for (int i = 0; i < 4; ++i) {
          if (pos_ok(b+Directions[i]) and cell(b+Directions[i]).type == Street) {
            int dist = days_to_move(id, round() + B[b].dist, cell(b+Directions[i]));
            if (B[b+Directions[i]].dist == -1 or B[b].dist + dist < B[b+Directions[i]].dist) {
              B[b+Directions[i]].dist = B[b].dist + dist;
              B[b+Directions[i]].dirs = B[b].dirs;
              q.push(make_pair(B[b+Directions[i]].dist,b+Directions[i]));
            }
            else if (B[b].dist + dist == B[b+Directions[i]].dist) {
              for (int j = 0; j < 4; ++j) B[b+Directions[i]].dirs[j] = B[b+Directions[i]].dirs[j] or B[b].dirs[j];
            }
          }
        }
      }
    }

    vector<double> v(4,0);
    for (Pos c : bonus) {
      for (int i = 0; i < 4; ++i) if (B[c].dirs[i]) v[i] += bonus_value(c, B[c].dist, id);
    }

    for (Pos c : weapons) {
      for (int i = 0; i < 4; ++i) if (B[c].dirs[i]) v[i] += weapon_value(c, B[c].dist, id);
    }

    for (Pos c : citizens) {
      for (int i = 0; i < 4; ++i) if (B[c].dirs[i]) v[i] += citizen_value(c, B[c].dist, id);
    }
    
    for (int i = 0; i < 4; ++i) if (not pos_ok(a+Directions[i]) or cell(a+Directions[i]).type != Street) v[i] = -1;

    int max = random(0,4);
    for (int i = 0; i < 4; ++i) {
      if (v[i] > v[max]) max = i;
    }
    move(id, Directions[max]);

//            ******DEBUG******
/*
    vector<char> aux = {'v','>','^','<'};
    cerr << a << endl;
    for (int i = 0; i < board_rows(); ++i) {
      for (int ii = 0; ii < board_cols(); ++ii) {
        cerr << '|';
        if (B[Pos(i,ii)].dist >= 0 and B[Pos(i,ii)].dist < 10) cerr << 0;
        cerr << B[Pos(i,ii)].dist;
        if (B[Pos(i,ii)].dist != -1 and B[Pos(i,ii)].dist != 0) {
          for (int d = 0; d < int(B[Pos(i,ii)].dirs.size()); ++d) {
            if(B[Pos(i,ii)].dirs[d]) cerr << aux[d];
            else cerr << '.';
          }
        }
        else cerr << "....";
        cerr << '|';
      }
      cerr << endl;
    }
    for (Dir d : Directions) {
      cerr << d << ':' << v[d] << ' ';
    }
    cerr << endl << "max=" << v[max] << ' ' << max << endl;
*/
//            *****************
  }

  /**
   * Play method, invoked once per each round.
   */
  virtual void play () {
    vector<int> w = warriors(me());
    vector<int> b = builders(me());
    if (is_round_night(round())) numBarricades = 0;
    else if (numBarricades < 3) {
      for (auto i : b) {
        vector<int> randPerm = random_permutation(4);
        for (int j : randPerm) {
          if (pos_ok(citizen(i).pos + Directions[j]) and cell(citizen(i).pos + Directions[j]).type == Street and cell(citizen(i).pos + Directions[j]).id == -1) {
            int aux = 0;
            for (Dir d : Directions) {
              if (pos_ok(citizen(i).pos + Directions[j] + d) and cell(citizen(i).pos + Directions[j] + d).type == Street) ++aux;
            }
            if (aux == 2) {
              build(i, Directions[j]);
              ++numBarricades;
              break;
            }
          }
        }
      }
    }

    for (auto i : w) find(i);
    for (auto i : b) find(i);
  }
};


/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);