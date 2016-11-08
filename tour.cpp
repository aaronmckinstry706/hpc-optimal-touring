#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <string.h>
#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include <ctime>
#include <unistd.h>
#include <algorithm>

#define CHECK_INTERVAL 100000
#define TIME_ALLOWED 120

bool stopEarly = false;//determines when whole program should stop

template <typename S, typename T>
std::ostream& operator<<(std::ostream& os, const std::pair<S,T>& p) {
	return os << "(" << p.first << "," << p.second << ")";
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v) {
	std::string s;
	std::ostringstream oss(s);
	for (int i = 0; i < v.size(); ++i) {
		oss << v[i] << ' ';
	}
	s = oss.str();
	s.pop_back();
	os << s;
	return os;
}

int CLOSEST = 20;
time_t programRealStartTime;

int MAX_X = -1;
int MAX_Y = -1;
int MAX_SIZE = -1;

struct site {
	int n;
	int x;
	int y;
	int duration;
	double value;
};

std::ostream& operator<<(std::ostream& os,const struct site& site) {
	return os << "(n=" << site.n
			  << ",x=" << site.x
			  << ",y=" << site.y
			  << ",dur=" << site.duration
			  << ",val=" << site.value
			  << ")";
}

struct times {
	int n;
	int day;
	int open;
	int close;
	times() {n = -1;}
	bool invalid() {return n < 0;}
};

std::ostream& operator<<(std::ostream& os,const struct times& times) {
	return os << "(n=" << times.n
			  << ",day=" << times.day
			  << ",open=" << times.open
			  << ",close=" << times.close
			  << ")";
}

struct cluster {
	double x;
	double y;
};

std::ostream& operator<<(std::ostream& os, const struct cluster& c) {
	return os << "(x=" << c.x
			  << ",y=" << c.y
			  << ")";
}

double distance(struct cluster c, struct site s) {
	int sum = pow(abs(c.x - s.x), 2);
	sum += pow(abs(c.y - s.y), 2);
	return sqrt(sum);
}

int parse_input(char *filename, std::vector<struct site> *sites, std::vector<struct times> *site_times) {
	
	std::ifstream in(filename);
	std::string line;

	bool parse_locations = 1;
	int num_days = 0;
	while (std::getline(in, line)) {
		std::istringstream iss(line);
		if (line.compare("site avenue street desiredtime value") == 0) {
			continue;
		}
		if (line.compare("site day beginhour endhour") == 0) {
			parse_locations = 0;
			continue;
		}
		if (line.compare("    ") == 0) {
			continue;
		}
		if (parse_locations) {
			struct site s;
			iss >> s.n;
			iss >> s.x;
			iss >> s.y;
			iss >> s.duration;
			iss >> s.value;
			sites->push_back(s);
			
			if (s.x > MAX_X)
				MAX_X = s.x;
			if (s.y > MAX_Y)
				MAX_Y = s.y;
			
		} else {
			struct times t;
			iss >> t.n;
			iss >> t.day;
			iss >> t.open;
			iss >> t.close;
			site_times->push_back(t);
			
			if (t.day > num_days)
				num_days = t.day;
		}
	}
	MAX_SIZE = std::max(MAX_X, MAX_Y);
	return num_days;
}

std::vector<struct cluster> get_cluster_centers(int num_clusters, std::vector<struct site> sites) {

	std::vector<struct cluster> clusters;
	for (int i = 0; i < num_clusters; i++) {
		double x, y;
		if (i < num_clusters/2) {
			if (num_clusters % 2 == 0) {
				x = (i * 2 * MAX_SIZE / num_clusters) + (MAX_SIZE / num_clusters);
				y = MAX_SIZE / 4;
			} else {
				x = (i * 2 * MAX_SIZE / (num_clusters-1)) + (MAX_SIZE / (num_clusters-1));
				y = MAX_SIZE / 4;
			}
		} else {
			if (num_clusters % 2 == 1) {
				x = ((i - (num_clusters/2)) * 2 * MAX_SIZE / (num_clusters+1)) + (MAX_SIZE / (num_clusters+1));
				y = 3 * MAX_SIZE / 4;
			} else {
				x = ((i - (num_clusters/2)) * 2 * MAX_SIZE / num_clusters) + (MAX_SIZE / num_clusters);
				y = 3 * MAX_SIZE / 4;
			}
		}
		struct cluster c;
		c.x = x;
		c.y = y;
		clusters.push_back(c);
	}
	int site_relationship[sites.size()];
	
	// run for K steps to settle cluster locations
	// or you can use while loop to see when they stop moving, but shouldn't matter too much
	for (int k = 0; k < 10; k++) {
		// assign
		for (int j = 0; j < sites.size(); j++) {
			int closest = -1;
			double dist = 1000;
			for (int i = 0; i < clusters.size(); i++) {
				double tmp_dist = distance(clusters[i], sites[j]);
				if (tmp_dist < dist) {
					dist = tmp_dist;
					closest = i;
				}
			}
			site_relationship[j] = closest;
		}

		// recalculate centroid
		for (int i = 0; i < clusters.size(); i++) {
			int count = 0;
			int new_x = 0;
			int new_y = 0;
			for (int j = 0; j < sites.size(); j++) {
				if (site_relationship[j] == i) {
					new_x += sites[j].x;
					new_y += sites[j].y;
					count++;
				}
			}
			if (count > 0) {
				clusters[i].x = 1.0 * new_x / count;
				clusters[i].y = 1.0 * new_y / count;
			}
		}
	}
	return clusters;
}

std::vector<struct site> get_nearby_sites_to_cluster(struct cluster c, std::vector<struct site> sites) {

	struct site_has_id {
		int id;
		site_has_id(int i) : id(i) {}
		bool operator()(struct site s) {
			return s.n == id;
		}
	};

	std::vector<std::pair<double, int> > site_distances;
	for (auto const& s : sites) {
		double dist = distance(c, s);
		std::pair<double, int> p(dist, s.n);
		site_distances.push_back(p);
	}
	
	std::vector<struct site> closest;
	std::sort(site_distances.begin(), site_distances.end());
	int count = 0;
	for (auto const& p : site_distances) {
		auto it = std::find_if(sites.begin(), sites.end(), site_has_id(p.second));
		closest.push_back(*it);
		if (++count == CLOSEST)
			break;
	}
	
	return closest;
}

int time_finished_visiting(struct site site,
						   struct times openHours,
						   std::pair<int,int> curPos,
						   int curTime) {
	int travelTime;
	if (curTime == 0)
		travelTime = openHours.open*60;
	else {
		travelTime = abs(site.x - curPos.first) + abs(site.y - curPos.second);
		travelTime = std::max(openHours.open*60-curTime,travelTime);
	}
	int visitTime = site.duration;
	int finishedTime = curTime + travelTime + visitTime;
	if (finishedTime > openHours.close*60)
		return -1;
	else
		return finishedTime;
}

int tick = 0;

std::pair<double,std::vector<int> > optimalPath(
					std::vector<struct site>& sites,
					std::vector<struct times>& times,
					std::vector<bool>& used,
					int curTime = 0,
					std::pair<int,int> curPos = std::pair<int,int>(0,0),
					int depth = 0) {
	
	double bestValue = 0;
	std::vector<int> bestPath;
	
	if (!stopEarly) {
	
	bool foundChild = false;
	for (int i = 0; i < sites.size(); ++i) {
		if (used[i] || times[i].invalid()) continue;
		
		int nextTime = time_finished_visiting(sites[i],times[i],curPos,curTime);
		std::pair<int,int> nextPosition = std::pair<int,int>(sites[i].x,sites[i].y);
		if (nextTime != -1) {
			foundChild = true;
			
			used[i] = true;
			std::pair<double,std::vector<int> > value_remainingPath =
				optimalPath(sites, times, used, nextTime, nextPosition, depth+1);
			used[i] = false;
			
			double totalValue = value_remainingPath.first + sites[i].value;
			std::vector<int>& path = value_remainingPath.second;
			
			if (totalValue > bestValue) {
				bestPath.clear();
				bestPath.push_back(i);
				bestPath.insert(bestPath.end(),path.begin(),path.end());
				bestValue = totalValue;
			}
			if (stopEarly) break;
		}
	}
	if (!foundChild) {
		tick = (tick+1)%CHECK_INTERVAL;
		if (tick%CHECK_INTERVAL == 0) {
			//check time remaining
			if (time(0) - programRealStartTime >= TIME_ALLOWED - 5)
				stopEarly = true;
		}
	}
	
	}//end stopEarly if-statement
	
	return std::pair<double,std::vector<int> >(bestValue,bestPath);
}

// function will consider any site close to cluster_order[i][k] for day k
// as long as it is not in string path, then update path
// return value for that day
std::pair<double,std::vector<int> > get_value_around_cluster_for_day(
		std::vector<struct site>& sites, 
		std::vector<struct times>& times, 
		struct cluster c,
		int day,
		std::vector<bool>& globalUsed) {
	
	struct times_has_day_id {
		int day;
		int id;
		times_has_day_id(int d, int i) : day(d),id(i) {}
		bool operator()(struct times t) {return t.n == id && t.day == day;}
	};
	
	double value = 0.0;
	std::vector<struct site> nearby = get_nearby_sites_to_cluster(c, sites);
	std::vector<struct times> nearbyTimes;
	for (auto const& s : nearby) {
		auto it = std::find_if(times.begin(),times.end(),times_has_day_id(day+1,s.n));
		if (it != times.end()) {
			nearbyTimes.push_back(*it);
		}
	}
	
	std::vector<bool> used(nearby.size(),false);
	for (int i = 0; i < used.size(); ++i)
		if (globalUsed[nearby[i].n])
			used[i] = true;
	
	std::pair<double,std::vector<int> > bestPath = optimalPath(nearby,nearbyTimes,used);
	
	//switch back to global id-indexing (site.n), rather than indexing into nearby[]
	for (int& i : bestPath.second) {
		i = nearby[i].n;
		globalUsed[i] = true;
	}
	
	return bestPath;
}

int myrandom(int i) { return std::rand()%i; }

int main(int argc, char *argv[]) {
	std::srand( unsigned ( std::time(0) ) );
	
	programRealStartTime = time(0);
	
	std::vector<struct site> sites;
	std::vector<struct times> site_times;
	int days = parse_input(argv[1], &sites, &site_times);
	
	struct lexical_cluster_comparison {
		lexical_cluster_comparison() {}
		bool operator()(const struct cluster& c1, const struct cluster& c2) {
			return c1.x < c2.x || (c1.x == c2.x && c1.y < c2.y);
		}
	};
	std::vector<struct cluster> clusters = get_cluster_centers(days, sites);
	std::sort(clusters.begin(),clusters.end(),lexical_cluster_comparison());
	
	std::vector<int> best_tour;
	double best_value = 0;
	

	do {
		std::vector<bool> globalUsed(200,false);
		std::vector<int> globalBestPath;
		
		double value = 0;
		for (int k = 0; k < days; k++) {
			std::pair<double, std::vector<int> > bestPathInCluster =
				get_value_around_cluster_for_day(
					sites, site_times, clusters[k], k, globalUsed);
			
			value += bestPathInCluster.first;
			globalBestPath.insert(globalBestPath.end(),
						  bestPathInCluster.second.begin(),
						  bestPathInCluster.second.end());
			
			if (stopEarly) break;
		}
		if (value > best_value) {
			best_value = value;
			best_tour = globalBestPath;
		}
		if (stopEarly) break;
		
		std::random_shuffle(clusters.begin(),clusters.end(), myrandom);
	} while (true);
	
	std::cout << best_tour << std::endl;

	return 0;
}

