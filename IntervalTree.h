#ifndef __INTERVAL_TREE_H
#define __INTERVAL_TREE_H

#include <vector>
#include <algorithm>
#include <iostream>

using namespace std;


template <class T, typename K = int>
class Interval {
public:
    K start;
    K stop;
    T value;
    Interval(K s, K e, const T& v)
        : start(s)
        , stop(e)
        , value(v)
    { }
};

template <class T, typename K>
K intervalStart(const Interval<T,K>& i) {
    return i.start;
}

template <class T, typename K>
K intervalStop(const Interval<T,K>& i) {
    return i.stop;
}

template <class T, typename K>
ostream& operator<<(ostream& out, Interval<T,K>& i) {
    out << "Interval(" << i.start << ", " << i.stop << "): " << i.value;
    return out;
}

template <class T, typename K = int>
class IntervalStartSorter {
public:
    bool operator() (const Interval<T,K>& a, const Interval<T,K>& b) {
        return a.start < b.start;
    }
};


template <class T, typename K>
class IntervalTreeView;


template <class T, typename K = int>
class IntervalTree {

public:
	friend class IntervalTreeView<T, K>;
    typedef Interval<T,K> interval;
    typedef vector<interval> intervalVector;
    typedef IntervalTree<T,K> intervalTree;
    
    intervalVector intervals;
    intervalTree* left;
    intervalTree* right;
	intervalTree* parent;
    K center;

    IntervalTree<T,K>(void)
        : left(NULL)
        , right(NULL)
		, parent(NULL)
        , center(0)
    { }

    IntervalTree<T,K>(const intervalTree& other) {
        center = other.center;
        intervals = other.intervals;
		parent = other.parent;
        if (other.left) {
            left = new intervalTree();
            *left = *other.left;
        } else {
            left = NULL;
        }
        if (other.right) {
            right = new intervalTree();
            *right = *other.right;
        } else {
            right = NULL;
        }
    }

    IntervalTree<T,K>& operator=(const intervalTree& other) {
        center = other.center;
        intervals = other.intervals;
		parent = other.parent;
        if (other.left) {
            left = new intervalTree();
            *left = *other.left;
        } else {
            left = NULL;
        }
        if (other.right) {
            right = new intervalTree();
            *right = *other.right;
        } else {
            right = NULL;
        }
        return *this;
    }

    IntervalTree<T,K>(
            intervalVector& ivals,
			intervalTree* parent = NULL,
            unsigned int depth = 16,
            unsigned int minbucket = 64,
            K leftextent = 0,
            K rightextent = 0,
            unsigned int maxbucket = 512
            )
        : left(NULL)
        , right(NULL)
		, parent(parent)
    {

        --depth;
        if (depth == 0 || (ivals.size() < minbucket && ivals.size() < maxbucket)) {
            intervals = ivals;
        } else {
            if (leftextent == 0 && rightextent == 0) {
                // sort intervals by start
                IntervalStartSorter<T,K> intervalStartSorter;
                sort(ivals.begin(), ivals.end(), intervalStartSorter);
            }

            K leftp;
            K rightp;
            
            if (leftextent || rightextent) {
                leftp = leftextent;
                rightp = rightextent;
            } else {
                leftp = ivals.front().start;
                vector<K> stops;
                stops.resize(ivals.size());
                transform(ivals.begin(), ivals.end(), stops.begin(), intervalStop<T,K>);
                rightp = *max_element(stops.begin(), stops.end());
            }

            //centerp = ( leftp + rightp ) / 2;
            K centerp = ivals.at(ivals.size() / 2).start;
            center = centerp;

            intervalVector lefts;
            intervalVector rights;

            for (typename intervalVector::iterator i = ivals.begin(); i != ivals.end(); ++i) {
                interval& interval = *i;
                if (interval.stop < center) {
                    lefts.push_back(interval);
                } else if (interval.start > center) {
                    rights.push_back(interval);
                } else {
                    intervals.push_back(interval);
                }
            }

            if (!lefts.empty()) {
                left = new intervalTree(lefts, this, depth, minbucket, leftp, centerp);
            }
            if (!rights.empty()) {
                right = new intervalTree(rights, this, depth, minbucket, centerp, rightp);
            }
        }
    }

    void findOverlapping(K start, K stop, intervalVector& overlapping) {
        if (!intervals.empty() && ! (stop < intervals.front().start)) {
            for (typename intervalVector::iterator i = intervals.begin(); i != intervals.end(); ++i) {
                interval& interval = *i;
                if (interval.stop >= start && interval.start <= stop) {
                    overlapping.push_back(interval);
                }
            }
        }

        if (left && start <= center) {
            left->findOverlapping(start, stop, overlapping);
        }

        if (right && stop >= center) {
            right->findOverlapping(start, stop, overlapping);
        }

    }

    void findContained(K start, K stop, intervalVector& contained) {
        if (!intervals.empty() && ! (stop < intervals.front().start)) {
            for (typename intervalVector::iterator i = intervals.begin(); i != intervals.end(); ++i) {
                interval& interval = *i;
                if (interval.start >= start && interval.stop <= stop) {
                    contained.push_back(interval);
                }
            }
        }

        if (left && start <= center) {
            left->findContained(start, stop, contained);
        }

        if (right && stop >= center) {
            right->findContained(start, stop, contained);
        }

    }

    ~IntervalTree(void) {
        // traverse the left and right
        // delete them all the way down
        if (left) {
            delete left;
        }
        if (right) {
            delete right;
        }
    }

};


template <class T, typename K = int>
class IntervalTreeView {
private:
	K start;
	K stop;
	vector< IntervalTree<T, K>* > toAnalize;
	IntervalTree<T, K>* currentTree;
	size_t currentIndex;
public:
	IntervalTreeView(
		IntervalTree<T, K> *tree, K start, K stop)
		: start(start)
		, stop(stop)
		, currentTree(NULL)
		, currentIndex(-1)
	{
		currentTree = tree;
		currentIndex = 0;
	}

	bool at_end()
	{
		return currentTree == NULL;
	}

	Interval<T, K>* get_interval()
	{
		return &(currentTree->intervals[currentIndex]);
	}

	T* get_element()
	{
		return &(get_interval()->value);
	}

	void move_next() {
		if(at_end())
			throw new exception("no more elements");
		Interval<T, K>* interval;
		do
		{
			if(currentIndex < currentTree->intervals.size() - 1)
				currentIndex++;
			else
			{
				if (currentTree->left && start <= currentTree->center)
					toAnalize.push_back(currentTree->left);

				if (currentTree->right && stop >= currentTree->center)
					toAnalize.push_back(currentTree->right);

				do
				{
					if(toAnalize.size() == 0)
					{
						currentTree = NULL;
						currentIndex = -1;
					}
					else
					{
						currentTree = toAnalize.back();
						currentIndex = 0;
						toAnalize.pop_back();
					}
				}
				while(currentTree != NULL && currentTree->intervals.size() == 0);
			}
			if(currentTree != NULL)
				interval = get_interval();
		}
		while(currentTree != NULL && 
			!(stop >= interval->start && start <= interval->stop));
    }
};

#endif
