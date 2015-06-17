#include <iostream>
#include <cstring>

using namespace std;

int maze [100][100];

struct vector
{
    int x, y;

    vector (int xi, int yi) :
        x(xi), y(yi) {}

    vector operator+ (vector const &r)
    {
        vector result (*this);
        result.x += r.x;
        result.y += r.y;
        return result;
    }

    vector &operator+= (vector const &r)
    {
        x += r.x;
        y += r.y;
        return *this;
    }
};

void turn_right (vector &direction)
{
    if (direction.x == 1 && direction.y == 0)
        direction.x = 0, direction.y = 1;

    else if (direction.x == 0 && direction.y == 1)
        direction.x = -1, direction.y = 0;

    else if (direction.x == -1 && direction.y == 0)
        direction.x = 0, direction.y = -1;

    else if (direction.x == 0 && direction.y == -1)
        direction.x = 1, direction.y = 0;
}

int main (int argc, char **argv)
{
    std::memset (maze, 0, sizeof (int) * 100 * 100);

    int M, N, step = 1, tries = 0;

    vector direction (1, 0);
    vector current (0, 0);
    vector next (0, 0);

    std::cin >> N >> M;

    while (tries < 4)
    {
        next = current + direction;
        if (next.x < M && next.y < N &&
            next.x >= 0 && next.y >= 0 &&
            maze[next.x][next.y] == 0)
        {
            std::cout << "c: " << current.x << " " << current.y << std::endl;
            std::cout << "d: " << direction.x << " " << direction.y << std::endl;
            std::cout << "n: " << next.x << " " << next.y << std::endl;
            std::cout << "---" << std::endl;
            maze[current.x][current.y] = step++;
            current = next;
            tries = 0;
        }
        else
        {
            std::cout << "blocked" << std::endl;
            tries ++;
        }

        turn_right (direction);
    }

    for (int i=0; i < M; ++i)
    {
        for (int j=0; j < N; ++j)
            std::cout << " " << maze[j][i];
        std::cout << std::endl;
    }

    std::cout << step << std::endl;

    return 0;
}
