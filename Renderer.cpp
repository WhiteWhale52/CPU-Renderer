#include <vector>
#include <fstream>
#include <cstdint>
#include <limits>
#include <cmath>
#include <iostream>
#include "LinearAlgebra.h"
#include <algorithm> 

const float PI = 3.14159f;

void print(char* string) {
	std::cout << string << "\n";
}
void print(float string) {
	std::cout << string << "\n";
}
void print(int string) {
	std::cout << string;
}

struct Vertex {
	Vector4 position;
	Vector3 normal;
	Vector2 uv;
};

struct Triangle { Vertex v0, v1, v2; };

struct Mesh { std::vector<Vertex> verts; std::vector<uint32_t> idx; };

struct VSOut {
	Vector4 clip;
	Vector3 normal;
	Vector2 uv;
	float inverseW = 0;
};


struct Color {
	uint8_t r, g, b, a;
};

struct Image {
	int width, height;

	std::vector<uint32_t> pixels;
	Image(int W, int H) : width(W), height(H), pixels(W* H, 0xff000000) {}
	
	void set(int x, int y, Color c) {
		if (x < 0 || x >= width || y < 0 || y >= height) return;
		pixels[y * width + x] = (c.a << 24) | (c.r << 16) | (c.g << 8) | c.b;
	}
	void save_ppm(const char* path) {
		std::ofstream out(path, std::ios::binary);
		out << "P6\n" << width << " " << height << "\n255\n";

		for(auto p : pixels){
			uint8_t r = (p >> 16) & 255, g = (p >> 8) & 255, b = p & 255;
				out.write(reinterpret_cast<char*>(&r), 1);
				out.write(reinterpret_cast<char*>(&g), 1);
				out.write(reinterpret_cast<char*>(&b), 1);
		}
		out.close();
	}
};


struct ScreenV {
	float x, y;
	float depth;
	Vector3 normalOverW;
	Vector2 uvOverW;
	float invW;
};

struct RasterTriangle {
	ScreenV v0, v1, v2;
};

struct DepthBuffer {
	int width, height;
	std::vector<float> zBuffer;
	DepthBuffer(int w, int h): width(w), height(h), zBuffer(width*height, std::numeric_limits<float>::infinity()){}
};

struct Texture {
	int WIDTH, HEIGHT; std::vector<uint32_t> rgba;
	Color Sample(float u, float v) const {
		u = u - std::floor(u);
		v = v - std::floor(v);
		int x = std::clamp(int(u * WIDTH), 0, WIDTH - 1);
		int y = std::clamp(int(v * HEIGHT), 0, HEIGHT - 1);
		uint32_t p = rgba[y * WIDTH + x];
		return { (uint8_t)(p >> 16), (uint8_t)(p >> 8), (uint8_t)p, (uint8_t)(p >> 24) };
	}
};

static VSOut vertShader(const Vertex& v, const Matrix4x4& ModelToWorld, const Matrix4x4& WorldToView, const Matrix4x4& ViewToClip) {
	Matrix4x4 MVP = mul(ViewToClip, mul(WorldToView, ModelToWorld));
	VSOut o;
	Vector4 position = v.position;
	o.clip = mul(MVP, position);
	std::cout << "O.Clip.x = " << o.clip.x << "\n";
	std::cout << "O.Clip.y = " << o.clip.y << "\n";
	std::cout << "O.Clip.z = " << o.clip.z << "\n";
	std::cout << "O.Clip.w = " << o.clip.w << "\n";
	o.normal = v.normal;
	o.uv = v.uv;
	o.inverseW = 1.0f / o.clip.w;
	print(o.inverseW);
	std::cout << "---------------------" << "\n";
	return o;
}


inline bool IsTopLeft(float ex, float ey) {
	return (ey < 0) || (ey == 0 && ex > 0);
};

void DrawTriangle(const RasterTriangle& R, Image& image, DepthBuffer& depthBuffer, Texture& texture) {
	// Bounding Box
	float minX = std::floor(std::min({ R.v0.x,R.v1.x, R.v2.x }));
	float maxX = std::ceil(std::max({ R.v0.x,R.v1.x, R.v2.x }));
	float minY = std::floor(std::min({ R.v0.y,R.v1.y, R.v2.y }));
	float maxY = std::ceil(std::max({ R.v0.y,R.v1.y, R.v2.y }));
	std::cout << "Min X = " << minX << "\n";
	std::cout << "Min Y = " << minY << "\n";
	std::cout << "Max X = " << maxX << "\n";
	std::cout << "Max Y = " << maxX << "\n";
	int x0 = std::max(0, (int)minX), x1 = std::min(image.width - 1, (int)maxX);
	int y0 = std::max(0, (int)minY), y1 = std::min(image.height - 1, (int)maxY);

	Vector2 a{ R.v0.x, R.v0.y }, b{ R.v1.x, R.v1.y }, c{ R.v2.x,R.v2.y };

	auto edge = [](float px, float py, Vector2 A, Vector2 B) {
		return (px - A.x) * (B.x - A.y) - (py - A.y) * (B.x - A.x);
	};

	float area = edge(a.x, a.y, b, c);
	if (area == 0) return;

	// For backface culling (optional)
	if (area < 0) return;

	for (int y = y0; y <= y1; ++y) {
		for (int x = x0; x <= x1; ++x) {
			float px = x + 0.5f, py = y + 0.5f;
			float e0 = edge(px, py, b, c);
			float e1 = edge(px, py, c, a);
			float e2 = edge(px, py, a, b);

			if (e0 < 0 || e1 < 0 || e2 < 0) continue;

			float w0 = e0 / area, w1 = e1 / area, w2 = e2 / area;

			float inverseW = w0 * R.v0.invW + w1 * R.v1.invW + w2 * R.v2.invW;

			float oneOverW = inverseW;

			float depth = (
				w0 * R.v0.depth * R.v0.invW +
				w1 * R.v1.depth * R.v1.invW +
				w2 * R.v2.depth * R.v2.invW) / oneOverW;

			int idX = y * depthBuffer.width + x;
			if (depth >= depthBuffer.zBuffer[idX]) continue;
			depthBuffer.zBuffer[idX] = depth;

			Vector3 normal = (R.v0.normalOverW * w0 + R.v1.normalOverW * w1 + R.v2.normalOverW * w2) * (1.0f / oneOverW);
			Vector2 uv = { (R.v0.uvOverW.x * w0 + R.v1.uvOverW.x * w1 + R.v2.uvOverW.x * w2),
				(R.v0.uvOverW.y * w0 + R.v1.uvOverW.y * w1 + R.v2.uvOverW.y * w2) };

			normal = normalise(normal);
			Vector3 L = normalise(Vector3{0.4f,0.8f,0.4f });
			float NdotL = std::max(0.0f, dot(normal, L));
			uint8_t c = (uint8_t)std::round(255.0f * NdotL);
			image.set(x, y, { c,c,c,255 });
			/*Color pixelColor = texture.Sample(uv.x, uv.y);
			uint8_t r = (uint8_t)std::round(pixelColor.r * NdotL);
			uint8_t g = (uint8_t)std::round(pixelColor.g * NdotL);
			uint8_t b = (uint8_t)std::round(pixelColor.b * NdotL);
			image.set(x, y, { r,g,b,255 });*/

		}
	}
	std::cin.get();
}

inline ScreenV ToScreen(const VSOut& v, int Width, int Height) {
	float x = (v.clip.x * v.inverseW * 0.5f + 0.5f) * Width;
	float y = (1.0f - (v.clip.y * v.inverseW * 0.5f + 0.5f)) * Height;
	float depth = v.clip.z * v.inverseW * 0.5f + 0.5f;
	ScreenV screenV{
		x,
		y,
		depth,
		v.normal * v.inverseW,
		{ v.uv.x * v.inverseW, v.uv.y * v.inverseW},
		v.inverseW
	};
	return screenV;
}




int main() {
	const int WIDTH = 800, HEIGHT = 600;
	Image img(WIDTH, HEIGHT);
	DepthBuffer zBuffer(WIDTH, HEIGHT);

	Vertex a{ {-0.7f, -0.6f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f,0.0f} };
	Vertex b{ { 0.6f, -0.6f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {1.0f,0.0f} };
	Vertex c{ { 0.0f,  0.6f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.5f,1.0f} };

	Matrix4x4 M = Matrix4x4::Identity();
	Matrix4x4 V = LookAt({ 0,0,2.5f }, { 0,0,0 }, { 0,1,0 });

	Matrix4x4 P = Perspective(60.0f * PI / 180.0f, float(WIDTH) / HEIGHT, 0.1f, 100.0f);

	VSOut A = vertShader(a, M, V, P);
	VSOut B = vertShader(b, M, V, P);
	VSOut C = vertShader(c, M, V, P);

	RasterTriangle T{ ToScreen(A, WIDTH,HEIGHT), ToScreen(B, WIDTH, HEIGHT), ToScreen(C, WIDTH, HEIGHT) };
	Texture tex{};
	DrawTriangle(T, img, zBuffer, tex);

	img.save_ppm("triangle.ppm");
	
}


//int main() {
//	const int W = 800, H = 600;
//	Image image(W, H);
//	DepthBuffer depthBuffer(W, H);
//
//	// Simple checker texture (procedural)
//	Texture tex; tex.WIDTH = 64; tex.HEIGHT = 64; tex.rgba.resize(64 * 64);
//	for (int y = 0;y < 64;++y) for (int x = 0;x < 64;++x) {
//		bool c = ((x / 8) ^ (y / 8)) & 1;
//		uint8_t v = c ? 255 : 30; tex.rgba[y * 64 + x] = (255 << 24) | (v << 16) | (v << 8) | v;
//	}
//
//	Vertex v0{ {-0.8f,-0.6f,0.0f, 1.0f}, {0,0,1}, {0,0} };
//	Vertex v1{ { 0.8f,-0.6f,0.0f, 1.0f}, {0,0,1}, {1,0} };
//	Vertex v2{ { 0.0f, 0.7f,0.0f, 1.0f}, {0,0,1}, {0.5f,1} };
//
//	Matrix4x4 M = Matrix4x4::Identity();
//	Matrix4x4 V = LookAt({ 0,0,2.2f }, { 0,0,0 }, { 0,1,0 });
//	Matrix4x4 P = Perspective(60.f * 3.14159f / 180.f, float(W) / H, 0.1f, 10.f);
//
//	VSOut A = vertShader(v0, M, V, P);
//	VSOut B = vertShader(v1, M, V, P);
//	VSOut C = vertShader(v2, M, V, P);
//
//	RasterTriangle T{ ToScreen(A,W,H), ToScreen(B,W,H), ToScreen(C,W,H) };
//
//	// Slightly modified shader in draw_triangle: sample tex with UV and modulate by N·L
//	// (use the 'texture' object in that scope)
//
//	DrawTriangle(T, image, depthBuffer, tex); // integrate texture sampling inside as shown
//
//	image.save_ppm("triangle.ppm");
//	return 0;
//}
