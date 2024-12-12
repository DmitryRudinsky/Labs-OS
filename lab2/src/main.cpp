#include <vector>
#include <cstdlib>
#include <cmath>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>

struct Point {
    double x;
    double y;
};

std::vector<Point> points;
std::vector<Point> centroids;
std::vector<int> assignments;
int maxThreads;
sem_t semaphore;

double distance(const Point& a, const Point& b) {
    return std::sqrt((a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y));
}

void* assignClusters(void* arg) {
    long startIdx = (long)arg;
    for (size_t i = startIdx; i < points.size(); i += maxThreads) {
        double minDist = std::numeric_limits<double>::max();
        int closestCluster = -1;

        for (size_t j = 0; j < centroids.size(); ++j) {
            double dist = distance(points[i], centroids[j]);
            if (dist < minDist) {
                minDist = dist;
                closestCluster = j;
            }
        }

        assignments[i] = closestCluster;
    }
    sem_post(&semaphore);
    pthread_exit(nullptr);
}

void* updateCentroids(void* arg) {
    long clusterIdx = (long)arg;
    double sumX = 0, sumY = 0;
    int count = 0;

    for (size_t i = 0; i < points.size(); ++i) {
        if (assignments[i] == clusterIdx) {
            sumX += points[i].x;
            sumY += points[i].y;
            count++;
        }
    }

    if (count > 0) {
        centroids[clusterIdx].x = sumX / count;
        centroids[clusterIdx].y = sumY / count;
    }
    sem_post(&semaphore);
    pthread_exit(nullptr);
}

void kMeansClustering(int k) {
    centroids.resize(k);
    for (int i = 0; i < k; ++i) {
        centroids[i] = points[rand() % points.size()];
    }
    assignments.resize(points.size(), -1);

    for (int iteration = 0; iteration < 100; ++iteration) {
        std::vector<pthread_t> threads(maxThreads);

        for (long i = 0; i < maxThreads; ++i) {
            sem_wait(&semaphore);
            pthread_create(&threads[i], nullptr, assignClusters, (void*)i);
        }
        for (int i = 0; i < maxThreads; ++i) {
            pthread_join(threads[i], nullptr);
        }

        for (long i = 0; i < k; ++i) {
            sem_wait(&semaphore);
            pthread_create(&threads[i], nullptr, updateCentroids, (void*)i);
        }
        for (int i = 0; i < k; ++i) {
            pthread_join(threads[i], nullptr);
        }
    }
}

int readInt() {
    char buffer[16];
    int bytesRead = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
    if (bytesRead <= 0) {
        write(STDERR_FILENO, "Error reading input\n", 20);
        exit(1);
    }
    buffer[bytesRead] = '\0';
    return std::atoi(buffer);
}

void writeString(const char* str) {
    write(STDOUT_FILENO, str, std::strlen(str));
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        writeString("Usage: ./program <maxThreads> <numClusters>\n");
        return 1;
    }

    maxThreads = std::atoi(argv[1]);
    int numClusters = std::atoi(argv[2]);

    writeString("Enter the number of points: ");
    int numPoints = readInt();

    points.resize(numPoints);
    writeString("Enter the coordinates (x y) for each point:\n");
    for (int i = 0; i < numPoints; ++i) {
        writeString("Point ");
        char indexBuffer[16];
        snprintf(indexBuffer, sizeof(indexBuffer), "%d: ", i + 1);
        writeString(indexBuffer);

        char inputBuffer[64];
        int bytesRead = read(STDIN_FILENO, inputBuffer, sizeof(inputBuffer) - 1);
        if (bytesRead <= 0) {
            write(STDERR_FILENO, "Error reading point\n", 20);
            exit(1);
        }
        inputBuffer[bytesRead] = '\0';
        sscanf(inputBuffer, "%lf %lf", &points[i].x, &points[i].y);
    }

    sem_init(&semaphore, 0, maxThreads);
    kMeansClustering(numClusters);

    writeString("Clusters:\n");
    for (size_t i = 0; i < centroids.size(); ++i) {
        char outputBuffer[128];
        snprintf(outputBuffer, sizeof(outputBuffer), "Cluster %zu: (%.2f, %.2f)\n",
                 i, centroids[i].x, centroids[i].y);
        writeString(outputBuffer);

        writeString("Points in Cluster:\n");
        for (size_t j = 0; j < points.size(); ++j) {
            if (assignments[j] == static_cast<int>(i)) {
                char pointBuffer[128];
                snprintf(pointBuffer, sizeof(pointBuffer), "  (%.2f, %.2f)\n", points[j].x, points[j].y);
                writeString(pointBuffer);
            }
        }
    }

    sem_destroy(&semaphore);
    return 0;
}