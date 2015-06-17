// Ryan McDougall
// GCC 4.8.2
// g++ -std=c++11 -Wno-write-strings tiles.cpp

#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <vector>
#include <array>
#include <deque>
#include <queue>
#include <map>

using std::cout;
using std::endl;

char *tiles1[] =
{
    "11", "23", "", "44", "11", "",
    "16", "36", "", "51", "71", "",
    "46", "26", "", "14", "68", 0
};

char *tiles2[] =
{
    "12", "34", "", "36", "15", "",
    "56", "78", "", "78", "90", "",
    "46", "26", "", "88", "01", "",
    "93", "54", 0
};


char *tiles3[] =
{
    "11", "12", "", "12", "93", "",
    "93", "99", "", "39", "98", "",
    "95", "86", "", "57", "66", "",
    "77", "57", "", "13", "24", "",
    "45", "47", "", "24", "39", 0
};

struct position
{
    int x, y;

    static position north (position const &p)
    {
        position result;
        result.x = p.x;
        result.y = p.y-1;
        return result;
    }

    static position south (position const &p)
    {
        position result;
        result.x = p.x;
        result.y = p.y+1;
        return result;
    }

    static position east (position const &p)
    {
        position result;
        result.x = p.x+1;
        result.y = p.y;
        return result;
    }

    static position west (position const &p)
    {
        position result;
        result.x = p.x-1;
        result.y = p.y;
        return result;
    }

};

struct box
{
    position nw, ne, se, sw;

    static box from_nw_corner (position const &p)
    {
        box result;
        result.nw.x = p.x;   result.nw.y = p.y;
        result.ne.x = p.x+1; result.ne.y = p.y;
        result.se.x = p.x+1; result.se.y = p.y+1;
        result.sw.x = p.x;   result.sw.y = p.y+1;
        return result;
    }

    static box from_ne_corner (position const &p)
    {
        box result;
        result.nw.x = p.x-1; result.nw.y = p.y;
        result.ne.x = p.x;   result.ne.y = p.y;
        result.se.x = p.x;   result.se.y = p.y+1;
        result.sw.x = p.x-1; result.sw.y = p.y+1;
        return result;
    }

    static box from_se_corner (position const &p)
    {
        box result;
        result.nw.x = p.x-1; result.nw.y = p.y-1;
        result.ne.x = p.x;   result.ne.y = p.y-1;
        result.se.x = p.x;   result.se.y = p.y;
        result.sw.x = p.x-1; result.sw.y = p.y;
        return result;
    }

    static box from_sw_corner (position const &p)
    {
        box result;
        result.nw.x = p.x;   result.nw.y = p.y-1;
        result.ne.x = p.x+1; result.ne.y = p.y-1;
        result.se.x = p.x+1; result.se.y = p.y;
        result.sw.x = p.x;   result.sw.y = p.y;
        return result;
    }
};

struct neighbor_box
{
    position north_west, north_east;
    position south_west, south_east;
    position east_north, east_south;
    position west_north, west_south;

    static neighbor_box expand (box const &b)
    {
        neighbor_box result;
        result.north_west = position::north (b.nw);
        result.north_east = position::north (b.ne);
        result.south_east = position::south (b.se);
        result.south_west = position::south (b.sw);
        result.east_north = position::east (b.ne);
        result.east_south = position::east (b.se);
        result.west_north = position::west (b.nw);
        result.west_south = position::west (b.sw);
        return result;
    }
};

struct tile
{
    char nw, ne, se, sw;

    void rotate ()
    {
        std::swap (nw, ne);
        std::swap (nw, se);
        std::swap (nw, sw);
    }
};

using matrix = std::deque<std::deque<char>>;
using lookup = std::map<char, std::vector<position>>;

struct board
{
    int link = 0;
    matrix table;
    lookup jump;

    void grow_north (int amount)
    {
        while (amount--)
        {
            ssize_t width = table.size()? table[0].size() : 0;
            table.push_front (matrix::value_type (width, 0));

            for (auto &map : jump)
                for (auto &position : map.second)
                    position.y++; // shift coordinates
        }
    }

    void grow_south (int amount)
    {
        while (amount--)
        {
            ssize_t width = table.size()? table[0].size() : 0;
            table.push_back (matrix::value_type (width, 0));
        }
    }

    void grow_east (int amount)
    {
        while (amount--)
        {
            for (auto &row : table)
                row.push_back ({});
        }
    }

    void grow_west (int amount)
    {
        while (amount--)
        {
            for (auto &row : table)
                row.push_front ({});

            for (auto &map : jump)
                for (auto &position : map.second)
                    position.x++; // shift coordinates
        }
    }

    bool in_bounds (position const &p)
    {
        return p.y >= 0 && p.y < table.size() &&
               p.x >= 0 && p.x < table[0].size();
    }

    char character_at (position const &p)
    {
        return table[p.y][p.x];
    }

    bool fits_in (box const &area, tile const &tile)
    {
        auto nw = in_bounds (area.nw)? character_at (area.nw) : 0;
        auto ne = in_bounds (area.ne)? character_at (area.ne) : 0;
        auto se = in_bounds (area.se)? character_at (area.se) : 0;
        auto sw = in_bounds (area.sw)? character_at (area.sw) : 0;
        auto has_room = nw == ne && ne == se && se == sw && sw == 0;

        neighbor_box n = neighbor_box::expand (area);
        auto nwn = in_bounds (n.north_west)? character_at (n.north_west) : 0;
        auto nen = in_bounds (n.north_east)? character_at (n.north_east) : 0;
        auto sen = in_bounds (n.south_east)? character_at (n.south_east) : 0;
        auto swn = in_bounds (n.south_west)? character_at (n.south_west) : 0;
        auto wnn = in_bounds (n.west_north)? character_at (n.west_north) : 0;
        auto enn = in_bounds (n.east_north)? character_at (n.east_north) : 0;
        auto esn = in_bounds (n.east_south)? character_at (n.east_south) : 0;
        auto wsn = in_bounds (n.west_south)? character_at (n.west_south) : 0;

        auto connects =
            (tile.nw == nwn || nwn == 0) && (tile.nw == wnn || wnn == 0) &&
            (tile.ne == nen || nen == 0) && (tile.ne == enn || enn == 0) &&
            (tile.se == sen || sen == 0) && (tile.se == esn || esn == 0) &&
            (tile.sw == swn || swn == 0) && (tile.sw == wsn || wsn == 0);

        auto connect_count =
            (tile.nw == nwn) + (tile.nw == wnn) +
            (tile.ne == nen) + (tile.ne == enn) +
            (tile.se == sen) + (tile.se == esn) +
            (tile.sw == swn) + (tile.sw == wsn);

        return has_room && connects && connect_count >= 2;
    }

    void place_in (box const &area, tile const &tile)
    {
        box b = area;

        ssize_t height = table.size();
        ssize_t width = height? table[0].size() : 0;

        int amount_north = (b.nw.y < 0)? 0 - b.nw.y : 0;
        int amount_south = (b.se.y >= height)? b.se.y - height + 1 : 0;
        int amount_east  = (b.se.x >= width)? b.se.x - width + 1 : 0 ;
        int amount_west  = (b.nw.x < 0)? 0 - b.nw.x : 0;

        grow_north (amount_north);
        grow_south (amount_south);
        grow_east (amount_east);
        grow_west (amount_west);

        height = table.size();
        width = height? table[0].size() : 0;

        b.nw.y += amount_north;   b.nw.x += amount_west;
        b.ne.y += amount_north;   b.ne.x += amount_west;
        b.se.y += amount_north;   b.se.x += amount_west;
        b.sw.y += amount_north;   b.sw.x += amount_west;

        table[b.nw.y][b.nw.x] = tile.nw;
        table[b.ne.y][b.ne.x] = tile.ne;
        table[b.se.y][b.se.x] = tile.se;
        table[b.sw.y][b.sw.x] = tile.sw;

        jump[tile.nw].push_back (b.nw);
        jump[tile.ne].push_back (b.ne);
        jump[tile.se].push_back (b.se);
        jump[tile.sw].push_back (b.sw);
    }

    void place (tile tile)
    {
        int tries = 0;
        int rotation = 0;
        bool placed = false;

        for (; rotation < 4; ++rotation)
        {
            for (auto p : jump[tile.nw])
            {
                ++tries;

                if (!placed)
                {
                    auto trial = box::from_nw_corner (position::east (p));
                    if (fits_in (trial, tile))
                    {
                        place_in (trial, tile);
                        placed = true;
                    }
                }

                if (!placed)
                {
                    auto trial = box::from_nw_corner (position::south (p));
                    if (fits_in (trial, tile))
                    {
                        place_in (trial, tile);
                        placed = true;
                    }
                }

                if (placed) break;
            }

            for (auto p : jump[tile.ne])
            {
                ++tries;

                if (!placed)
                {
                    auto trial = box::from_ne_corner (position::south (p));
                    if (fits_in (trial, tile))
                    {
                        place_in (trial, tile);
                        placed = true;
                    }
                }

                if (!placed)
                {
                    auto trial = box::from_ne_corner (position::west (p));
                    if (fits_in (trial, tile))
                    {
                        place_in (trial, tile);
                        placed = true;
                    }
                }

                if (placed) break;
            }

            for (auto p : jump[tile.se])
            {
                ++tries;

                if (!placed)
                {
                    auto trial = box::from_se_corner (position::west (p));
                    if (fits_in (trial, tile))
                    {
                        place_in (trial, tile);
                        placed = true;
                    }
                }

                if (!placed)
                {
                    auto trial = box::from_se_corner (position::north (p));
                    if (fits_in (trial, tile))
                    {
                        place_in (trial, tile);
                        placed = true;
                    }
                }

                if (placed) break;
            }

            for (auto p : jump[tile.sw])
            {
                ++tries;

                if (!placed)
                {
                    auto trial = box::from_sw_corner (position::north (p));
                    if (fits_in (trial, tile))
                    {
                        place_in (trial, tile);
                        placed = true;
                    }
                }

                if (!placed)
                {
                    auto trial = box::from_sw_corner (position::east (p));
                    if (fits_in (trial, tile))
                    {
                        place_in (trial, tile);
                        placed = true;
                    }
                }

                if (placed) break;
            }

            if (placed) break;

            tile.rotate();
        }

        if (!placed && tries == 0)
        {
            place_in (box::from_nw_corner ({0, 0}), tile);
            placed = true;
        }

        if (placed)
        {
            ++link;
            cout << "link " << link;

            if (rotation % 4)
                cout << " // note this tile had to be rotated degrees "
                    << rotation * 90 << " clockwise" << endl;
            else
                cout << endl;

            print();
            cout << endl;
        }
        else
            cout << "!!!!!" << endl;
    }

    void print () const
    {
        for (auto const &row : table)
        {
            for (auto const &ch : row)
                cout << ((ch != '\0')? ch : ' ');
            cout << endl;
        }
    }

};

std::queue<tile> load (char **tiles)
{
    std::queue<tile> result;

    for (int i=0; tiles[i];)
    {
        if (tiles[i][0] == '\0') i++;

        auto top = tiles[i++];
        auto bot = tiles[i++];

        auto nw = top[0];
        auto ne = top[1];
        auto sw = bot[0];
        auto se = bot[1];

        result.push ({nw, ne, se, sw});
    }

    return result;
}

void place (std::queue<tile> &tiles, board &board)
{
    cout << tiles.size() << " tiles" << endl;

    while (!tiles.empty())
    {
        board.place (tiles.front());
        tiles.pop();
    }
}

int main ()
{
    {
        board result;
        auto tiles = load (tiles1);
        place (tiles, result);
    }

    {
        board result;
        auto tiles = load (tiles2);
        place (tiles, result);
    }

    {
        board result;
        auto tiles = load (tiles3);
        place (tiles, result);
    }

    return 0;
}
