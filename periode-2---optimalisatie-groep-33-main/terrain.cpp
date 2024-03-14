#include "terrain.h"
#include "precomp.h"
#include <map>
#include <iostream>
#include <cmath>
#include <mutex>

namespace fs = std::filesystem;
namespace Tmpl8
{
    Terrain::Terrain()
    {
        //Load in terrain sprites
        grass_img = std::make_unique<Surface>("assets/tile_grass.png");
        forest_img = std::make_unique<Surface>("assets/tile_forest.png");
        rocks_img = std::make_unique<Surface>("assets/tile_rocks.png");
        mountains_img = std::make_unique<Surface>("assets/tile_mountains.png");
        water_img = std::make_unique<Surface>("assets/tile_water.png");


        tile_grass = std::make_unique<Sprite>(grass_img.get(), 1);
        tile_forest = std::make_unique<Sprite>(forest_img.get(), 1);
        tile_rocks = std::make_unique<Sprite>(rocks_img.get(), 1);
        tile_water = std::make_unique<Sprite>(water_img.get(), 1);
        tile_mountains = std::make_unique<Sprite>(mountains_img.get(), 1);


        //Load terrain layout file and fill grid based on tiletypes
        fs::path terrain_file_path{ "assets/terrain.txt" };
        std::ifstream terrain_file(terrain_file_path);

        
        if (terrain_file.is_open())
        {
            std::string terrain_line;

            std::getline(terrain_file, terrain_line);
            std::istringstream lineStream(terrain_line);

            int rows;

            lineStream >> rows;

            //for each row as long as row is smaller than rows
            for (size_t row = 0; row < rows; row++)
            {
                std::getline(terrain_file, terrain_line);

                //for each column as long as column is smaller than terrain_line.size
                for (size_t collumn = 0; collumn < terrain_line.size(); collumn++)
                {
                    //if toupper... is equal to a certain character, use one of the corresponding images for the tile
                    switch (std::toupper(terrain_line.at(collumn)))
                    {
                    case 'G':
                        tiles.at(row).at(collumn).tile_type = TileType::GRASS;
                        break;
                    case 'F':
                        tiles.at(row).at(collumn).tile_type = TileType::FORREST;
                        break;
                    case 'R':
                        tiles.at(row).at(collumn).tile_type = TileType::ROCKS;
                        break;
                    case 'M':
                        tiles.at(row).at(collumn).tile_type = TileType::MOUNTAINS;
                        break;
                    case 'W':
                        tiles.at(row).at(collumn).tile_type = TileType::WATER;
                        break;
                    default:
                        tiles.at(row).at(collumn).tile_type = TileType::GRASS;
                        break;
                    }
                }
            }
        }
        //if it can't find the terrain file return the following text
        else
        {
            std::cout << "Could not open terrain file! Is the path correct? Defaulting to grass.." << std::endl;
            std::cout << "Path was: " << terrain_file_path << std::endl;
        }

        //Instantiate tiles for path planning
        //for all tiles at (x, y) check if it's inside the accessible area, if so then tile can be used as a path
        for (size_t y = 0; y < tiles.size(); y++)
        {
            for (size_t x = 0; x < tiles.at(y).size(); x++)
            {
                tiles.at(y).at(x).position_x = x;
                tiles.at(y).at(x).position_y = y;

                if (is_accessible(y, x + 1)) { tiles.at(y).at(x).exits.push_back(&tiles.at(y).at(x + 1)); }
                if (is_accessible(y, x - 1)) { tiles.at(y).at(x).exits.push_back(&tiles.at(y).at(x - 1)); }
                if (is_accessible(y + 1, x)) { tiles.at(y).at(x).exits.push_back(&tiles.at(y + 1).at(x)); }
                if (is_accessible(y - 1, x)) { tiles.at(y).at(x).exits.push_back(&tiles.at(y - 1).at(x)); }
            }
        }
    }

    void Terrain::update()
    {
        //Pretend there is animation code here.. next year :)
    }

    void Terrain::draw(Surface* target) const
    {
        //for tile on the vertical axis
        for (size_t y = 0; y < tiles.size(); y++)
        {
            //for tile on the horizontal axis
            for (size_t x = 0; x < tiles.at(y).size(); x++)
            {
                int posX = (x * sprite_size) + HEALTHBAR_OFFSET;
                int posY = y * sprite_size;
                
                //for tiles at (x, y) draw the tile using recursion
                switch (tiles.at(y).at(x).tile_type)
                {
                case TileType::GRASS:
                    tile_grass->draw(target, posX, posY);
                    break;
                case TileType::FORREST:
                    tile_forest->draw(target, posX, posY);
                    break;
                case TileType::ROCKS:
                    tile_rocks->draw(target, posX, posY);
                    break;
                case TileType::MOUNTAINS:
                    tile_mountains->draw(target, posX, posY);
                    break;
                case TileType::WATER:
                    tile_water->draw(target, posX, posY);
                    break;
                default:
                    tile_grass->draw(target, posX, posY);
                    break;
                }
            }
        }
    }

    
    std::mutex MyLock;
    //Use Breadth-first search to find shortest route to the destination
    vector<vec2> Terrain::get_route(const Tank& tank, const vec2& target)
    {
        MyLock.lock();
        //Find start and target tile
        const size_t pos_x = tank.position.x / sprite_size;
        const size_t pos_y = tank.position.y / sprite_size;

        const size_t target_x = target.x / sprite_size;
        const size_t target_y = target.y / sprite_size;

        //Init queue with start tile
        std::queue<vector<TerrainTile*>> queue;
        queue.emplace();
        queue.back().push_back(&tiles.at(pos_y).at(pos_x));

        std::vector<TerrainTile*> visited; 

        bool route_found = false;
        vector<TerrainTile*> current_route;
        
        MyLock.unlock();

        
        //While queue is not empty and route hasn't been found
        while (!queue.empty() && !route_found)
        {
            std::mutex WhileLock;
            WhileLock.lock();
            //current_route is in front of the queue. we're checking this tile
            //Then remove first element from queue
            current_route = queue.front();
            queue.pop();
            TerrainTile* current_tile = current_route.back();
            
            //Check all exits, if target then done, else if unvisited push a new partial route
            for each (TerrainTile * exit in current_tile->exits)
            {

                //if exit coordinates == target coordinates, route is found and remove from queue
                if (exit->position_x == target_x && exit->position_y == target_y)
                {
                    current_route.push_back(exit);
                    route_found = true;
                    break;
                }
                //else if exit is not visited and the actual distance
                //make exit visited
                else if (!exit->visited)
                {
                    exit->visited = true;
                    visited.push_back(exit);
                    queue.push(current_route);
                    queue.back().push_back(exit);
                }
            }
            WhileLock.unlock();
        }

        //Reset tiles
        for each (TerrainTile * tile in visited)
        {
            tile->visited = false;
        }

        if (route_found)
        {
            //Convert route to vec2 to prevent dangling pointers
            std::vector<vec2> route;

            //foreach tile in current route -> call push_back method 
            for (TerrainTile* tile : current_route)
            {
                route.push_back(vec2((float)tile->position_x * sprite_size, (float)tile->position_y * sprite_size));
            }

            return route;
        }
        //else return list
        else
        {
            return  std::vector<vec2>();

        }

    }

    int Terrain::get_Manhattan_Dist(const TerrainTile* currPos, const size_t target_x, const size_t target_y)
    {
        //Get Manhattan Distance using DeltaX + DeltaY
        int deltaX = currPos->position_x - target_x;
        int deltaY = currPos->position_y - target_y;
        //We only work with ABSolutes, no negative values
        return std::abs(deltaX + deltaY);

    }

    //Dijkstra Algorithm to find shortest route to destination
    vector<vec2> Terrain::astar_get_route(const Tank& tank, const vec2& target)
    {

        //Find start and target tile
        const size_t pos_x = tank.position.x / sprite_size;
        const size_t pos_y = tank.position.y / sprite_size;

        const size_t target_x = target.x / sprite_size;
        const size_t target_y = target.y / sprite_size;

        //Init queue with start tile
        std::queue<vector<TerrainTile*>> queue;

        queue.emplace();
        queue.back().push_back(&tiles.at(pos_y).at(pos_x));
        //Get Manhattan distance from start to finish
        int Start_To_End = get_Manhattan_Dist(&tiles.at(pos_y).at(pos_x),target_x,target_y );

        //Map to insert all visited tiles along with their weights
        std::map<TerrainTile*, float> visited;

        bool route_found = false;
        vector<TerrainTile*> current_route;

        while (!queue.empty() && !route_found)
        {
            //Front of the queue becomes the current_route
            current_route = queue.front();

            //remove this tile, it's now in the current_route
            queue.pop();

            //current tile becomes the the tile at the back
            TerrainTile* current_tile = current_route.back();
      
            for each (TerrainTile* exit in current_tile->exits) 
            { 
                //Get Distance?
                int TileDist = get_Manhattan_Dist(current_tile, target_x, target_y);

                if (exit->position_x == target_x && exit->position_y == target_y)
                {
                    current_route.push_back(exit);
                    route_found = true;
                    break;
                }

                else if (!exit->visited)
                {
                    exit->visited = true;

                    //Calculate the tile's weight and insert them in the visited list
                    vec2 Tilespeedmod = vec2((float)exit->position_x, (float)exit->position_y);
                    float speedmod = get_speed_modifier(Tilespeedmod);
                    visited.insert(std::pair<TerrainTile*, float>(exit, speedmod));

                    queue.push(current_route);
                    queue.back().push_back(exit);
                }

            }
        }

#if 0
        while (!queue.empty() && !route_found)
        {
            //current_route is in front of the queue. we're checking this tile
            //Then remove first element from queue
            current_route = queue.front();
            queue.pop();
            TerrainTile* current_tile = current_route.back();

            //Check all exits, if target then done, else if unvisited push a new partial route
            for each (TerrainTile * exit in current_tile->exits)
            {

                //if exit coordinates == target coordinates, route is found and remove from queue
                if (exit->position_x == target_x && exit->position_y == target_y)
                {
                    current_route.push_back(exit);
                    route_found = true;
                    break;
                }
                //else if exit is not visited
                //mark exit visited
                else if (!exit->visited)
                {
                    exit->visited = true;

                    //Calculate the tile's weight and insert them in the visited list
                    vec2 Tilespeedmod = vec2((float)exit->position_x, (float)exit->position_y);
                    float speedmod = get_speed_modifier(Tilespeedmod);
                    visited.insert(std::pair<TerrainTile*, float>(exit, speedmod));

                    queue.push(current_route);

                    //QueueMap.
                    queue.back().push_back(exit);
                }
            }
        }
#endif
        //Reset tiles
        std::map<TerrainTile*, float>::iterator it = visited.begin();
        while (it != visited.end())
        {
            TerrainTile* tileReset = it->first;
            float TileWeight = it->second;

            tileReset->visited = false;

            it++;
        }

        if (route_found)
        {
            //Convert route to vec2 to prevent dangling pointers
            std::vector<vec2> route;

            //foreach tile in current route -> call push_back method 
            for (TerrainTile* tile : current_route)
            {
                route.push_back(vec2((float)tile->position_x * sprite_size, (float)tile->position_y * sprite_size));
            }

            return route;
        }
        //else return list
        else
        {
            return  std::vector<vec2>();
        }
        //TO DO: delete created lists after all tanks have a route for better memory management :) 
        
    }



    //TODO: Function not used, convert BFS to dijkstra and take speed into account next year :)
    float Terrain::get_speed_modifier(const vec2& position) const
    {
        const size_t pos_x = position.x ; // / sprite_size
        const size_t pos_y = position.y ; // / sprite_size

        switch (tiles.at(pos_y).at(pos_x).tile_type)
        {
        case TileType::GRASS:
            return 1.0f;
            break;
        case TileType::FORREST:
            return 0.5f;
            break;
        case TileType::ROCKS:
            return 0.75f;
            break;
        case TileType::MOUNTAINS:
            return 0.0f;
            break;
        case TileType::WATER:
            return 0.0f;
            break;
        default:
            return 1.0f;
            break;
        }
    }

    //Check if tile is acessible for tanks
    bool Terrain::is_accessible(int y, int x)
    {
        //Bounds check (if x is between 0 and terrain_width AND y is between 0 and terrain_height)
        if ((x >= 0 && x < terrain_width) && (y >= 0 && y < terrain_height))
        {
            //Inaccessible terrain check (then if tile is not a mountain or water, then tile is accessible)
            if (tiles.at(y).at(x).tile_type != TileType::MOUNTAINS && tiles.at(y).at(x).tile_type != TileType::WATER)
            {
                return true;
            }
        }
        //else not
        return false;
    }
}