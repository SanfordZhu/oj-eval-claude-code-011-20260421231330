#ifndef SJTU_PRIORITY_QUEUE_HPP
#define SJTU_PRIORITY_QUEUE_HPP

#include <cstddef>
#include <functional>
#include "exceptions.hpp"

namespace sjtu {

/**
 * @brief Priority queue implemented as a leftist (max-)heap.
 *
 * Exception safety:
 *   The Compare functor may throw. In mergeTrees() the pattern is:
 *       if (cmp(t1->value, t2->value)) { local swap of t1/t2 pointers }
 *       t1->right = mergeTrees(t1->right, t2, cmp);
 *       // leftist fixup + dist update
 *   Any structural mutation of the heap only happens AFTER the inner cmp
 *   call (and the subsequent recursion) has returned successfully. If cmp
 *   throws at any level, the exception propagates up the stack without any
 *   mutation being made at any enclosing level, so both input trees retain
 *   their original shape and content. This gives us strong exception safety
 *   "for free" on push / pop / merge without needing to clone the heap.
 */
template<typename T, class Compare = std::less<T>>
class priority_queue {
private:
	struct Node {
		T value;
		Node *left;
		Node *right;
		int dist; // s-value / null path length; leaf = 0, null = -1

		Node(const T &v) : value(v), left(nullptr), right(nullptr), dist(0) {}
	};

	Node *root;
	size_t sz;
	Compare cmp;

	static Node *cloneTree(const Node *o) {
		if (!o) return nullptr;
		Node *n = new Node(o->value);
		n->dist = o->dist;
		try {
			n->left = cloneTree(o->left);
		} catch (...) {
			delete n;
			throw;
		}
		try {
			n->right = cloneTree(o->right);
		} catch (...) {
			destroyTree(n->left);
			delete n;
			throw;
		}
		return n;
	}

	static void destroyTree(Node *n) {
		if (!n) return;
		destroyTree(n->left);
		destroyTree(n->right);
		delete n;
	}

	static int getDist(Node *n) { return n ? n->dist : -1; }

	// Destructively merge two leftist trees using cmp. On exception from cmp,
	// both input trees are left structurally unchanged (see class header
	// explanation for the invariant).
	static Node *mergeTrees(Node *t1, Node *t2, Compare &cmp) {
		if (!t1) return t2;
		if (!t2) return t1;
		// Max-heap: we want t1->value to be the larger/equal element.
		// cmp(a,b) means "a < b". If cmp(t1,t2) is true then t2 is larger.
		if (cmp(t1->value, t2->value)) {
			Node *tmp = t1; t1 = t2; t2 = tmp;
		}
		Node *newRight = mergeTrees(t1->right, t2, cmp);
		// --- all mutations below this line only execute on success ---
		t1->right = newRight;
		if (getDist(t1->left) < getDist(t1->right)) {
			Node *tmp = t1->left; t1->left = t1->right; t1->right = tmp;
		}
		t1->dist = getDist(t1->right) + 1;
		return t1;
	}

public:
	priority_queue() : root(nullptr), sz(0) {}

	priority_queue(const priority_queue &other)
		: root(nullptr), sz(0), cmp(other.cmp) {
		root = cloneTree(other.root);
		sz = other.sz;
	}

	~priority_queue() {
		destroyTree(root);
		root = nullptr;
		sz = 0;
	}

	priority_queue &operator=(const priority_queue &other) {
		if (this == &other) return *this;
		Node *newRoot = cloneTree(other.root);
		destroyTree(root);
		root = newRoot;
		sz = other.sz;
		cmp = other.cmp;
		return *this;
	}

	const T &top() const {
		if (!root) throw container_is_empty();
		return root->value;
	}

	void push(const T &e) {
		Node *newNode = new Node(e);
		Node *merged;
		try {
			merged = mergeTrees(root, newNode, cmp);
		} catch (...) {
			// cmp threw. Both root and newNode are structurally unchanged.
			delete newNode;
			throw;
		}
		root = merged;
		++sz;
	}

	void pop() {
		if (!root) throw container_is_empty();
		Node *l = root->left;
		Node *r = root->right;
		Node *merged;
		try {
			merged = mergeTrees(l, r, cmp);
		} catch (...) {
			// cmp threw. l and r are structurally unchanged, still reachable
			// from root. Nothing to do.
			throw;
		}
		Node *oldRoot = root;
		root = merged;
		delete oldRoot;
		--sz;
	}

	size_t size() const { return sz; }

	bool empty() const { return root == nullptr; }

	void merge(priority_queue &other) {
		if (this == &other) return;
		if (!other.root) return;
		Node *merged;
		try {
			merged = mergeTrees(root, other.root, cmp);
		} catch (...) {
			// Both trees are structurally unchanged.
			throw;
		}
		root = merged;
		sz += other.sz;
		other.root = nullptr;
		other.sz = 0;
	}
};

}

#endif
