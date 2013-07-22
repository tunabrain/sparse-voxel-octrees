#ifndef PLYLOADER_HPP_
#define PLYLOADER_HPP_

#include <vector>

#include "math/Vec3.hpp"

struct Vertex {
	Vec3 position, normal, color;

	Vertex(const Vec3 &p, const Vec3 &n, const Vec3 &c) :
		position(p), normal(n), color(c) {}
};

struct Triangle {
	Vertex v1, v2, v3;

	Triangle(const Vertex &_v1, const Vertex &_v2, const Vertex &_v3) :
		v1(_v1), v2(_v2), v3(_v3) {}
};

class PlyLoader {
	std::vector<Vertex> _verts;
	std::vector<Triangle> _tris;

public:
	PlyLoader(const char *path);

	const std::vector<Triangle> &tris() const {
		return _tris;
	}

	const std::vector<Vertex> &verts() const {
		return _verts;
	}
};

#endif /* OBJLOADER_HPP_ */
