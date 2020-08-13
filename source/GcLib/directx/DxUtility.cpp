#include "source/GcLib/pch.h"

#include "DxUtility.hpp"

using namespace gstd;
using namespace directx;

/**********************************************************
//ColorAccess
**********************************************************/
D3DCOLORVALUE ColorAccess::SetColor(D3DCOLORVALUE& value, D3DCOLOR color) {
	D3DXVECTOR4 _col = ToVec4Normalized(color);		//argb
	D3DXVECTOR4 colVec = D3DXVECTOR4(_col.y, _col.z, _col.w, _col.x);	//rgba
	value = (D3DCOLORVALUE&)ColorAccess::SetColor(colVec, color);
	return value;
}
D3DMATERIAL9 ColorAccess::SetColor(D3DMATERIAL9 mat, D3DCOLOR color) {
	D3DXVECTOR4 _col = ToVec4Normalized(color);			//argb
	__m128 col = { _col.y, _col.z, _col.w, _col.x };	//rgba
	__m128 dst;

	auto PerformMul = [&](D3DCOLORVALUE* src) {
		memcpy(&dst, src, sizeof(D3DCOLORVALUE));	//rgba
		dst = _mm_mul_ps(dst, col);
		//D3DXVECTOR4 cRes = ColorAccess::ClampColorPacked((D3DXVECTOR4&)dst);
		memcpy(src, &dst, sizeof(D3DCOLORVALUE));
	};
	PerformMul(&mat.Diffuse);
	PerformMul(&mat.Specular);
	PerformMul(&mat.Ambient);
	PerformMul(&mat.Emissive);

	return mat;
}
D3DXVECTOR4& ColorAccess::SetColor(D3DXVECTOR4& value, D3DCOLOR color) {
	__m128 v1 = (__m128&)value;					//argb
	D3DXVECTOR4 col = ToVec4Normalized(color);	//argb
	v1 = _mm_mul_ps(v1, (__m128&)col);
	value = ColorAccess::ClampColorPacked((D3DXVECTOR4&)v1);
	return value;
}
D3DCOLOR& ColorAccess::SetColor(D3DCOLOR& src, const D3DCOLOR& mul) {
	D3DXVECTOR4 mulFac = ToVec4Normalized(mul);
	__m128 vsrc = (__m128&)ToVec4(src);			//argb
	__m128 res = _mm_mul_ps(vsrc, (__m128&)(mulFac));
	__m128i argb = ColorAccess::ClampColorPackedM((D3DXVECTOR4&)res);
	int* _i = reinterpret_cast<int*>(&argb);
	src = D3DCOLOR_ARGB(_i[0], _i[1], _i[2], _i[3]);
	return src;
}
D3DCOLOR& ColorAccess::ApplyAlpha(D3DCOLOR& color, float alpha) {
	__m128 v1 = (__m128&)ToVec4(color);			//argb
	__m128 v2 = { alpha, alpha, alpha, alpha };
	v1 = _mm_mul_ps(v1, v2);
	__m128i argb = ColorAccess::ClampColorPackedM((D3DXVECTOR4&)v1);
	int* _i = reinterpret_cast<int*>(&argb);
	color = D3DCOLOR_ARGB(_i[0], _i[1], _i[2], _i[3]);
	return color;
}

D3DXVECTOR4 ColorAccess::ClampColorPacked(const D3DXVECTOR4& src) {
	__m128i ci = _mm_setr_epi32((int)src[0], (int)src[1], (int)src[2], (int)src[3]);
	return ColorAccess::ClampColorPacked(ci);
}
D3DXVECTOR4 ColorAccess::ClampColorPacked(const __m128i& src) {
	__m128i ci = ColorAccess::ClampColorPackedM(src);
	int* _i = reinterpret_cast<int*>(&ci);
	return D3DXVECTOR4((float)_i[0], (float)_i[1], (float)_i[2], (float)_i[3]);
}
__m128i ColorAccess::ClampColorPackedM(const D3DXVECTOR4& src) {
	__m128i ci = _mm_setr_epi32((int)src[0], (int)src[1], (int)src[2], (int)src[3]);
	return ColorAccess::ClampColorPackedM(ci);
}
__m128i ColorAccess::ClampColorPackedM(const __m128i& src) {
	__m128i cn = _mm_setr_epi32(0x00, 0x00, 0x00, 0x00);
	__m128i cx = _mm_setr_epi32(0xff, 0xff, 0xff, 0xff);
	cx = _mm_maskz_min_epi32(0xff, src, cx);
	return _mm_maskz_max_epi32(0xff, cx, cn);
}

D3DXVECTOR3& ColorAccess::RGBtoHSV(D3DXVECTOR3& color, int red, int green, int blue) {
	//[In]  RGB: (0 ~ 255, 0 ~ 255, 0 ~ 255)
	//[Out] HSV: (0 ~ 360, 0 ~ 255, 0 ~ 255)

	int cmax = std::max(std::max(red, green), blue);
	int cmin = std::min(std::min(red, green), blue);
	float delta = cmax - cmin;

	color.z = cmax;
	if (delta < 0.00001f) {
		color.x = 0;
		color.y = 0;
	}
	else if (cmax == 0) {
		color.x = NAN;
		color.y = 0;
	}
	else {
		color.y = (int)std::roundf(delta / cmax * 255.0f);

		if (red >= cmax)
			color.x = (green - blue) / delta;
		else if (green >= cmax)
			color.x = 2.0f + (blue - red) / delta;
		else
			color.x = 4.0f + (red - green) / delta;

		color.x = (int)Math::NormalizeAngleDeg(color.x * 60.0f);
	}

	return color;
}
D3DCOLOR& ColorAccess::HSVtoRGB(D3DCOLOR& color, int hue, int saturation, int value) {
	//Hue: 0 ~ 360
	//Sat: 0 ~ 255
	//Val: 0 ~ 255

	int hh = hue % 360;
	if (hh < 0) hh += 360;

	int i = hh / 60;
	float ff = (hh % 60) / 60.0f;
	float s = saturation / 255.0f;

	int p = value * (1.0f - s);
	int q = value * (1.0f - s * ff);
	int t = value * (1.0f - s * (1.0f - ff));

	auto GenColor = [](int r, int g, int b) -> D3DCOLOR {
		__m128i ci = _mm_setr_epi32(r, g, b, 0);
		int* _i = reinterpret_cast<int*>(&ci);
		ci = ColorAccess::ClampColorPackedM(ci);
		return D3DCOLOR_XRGB(_i[0], _i[1], _i[2]);
	};

	D3DCOLOR rgb = 0xffffffff;
	switch (i) {
	case 0: rgb = GenColor(value, t, p); break;
	case 1: rgb = GenColor(q, value, p); break;
	case 2: rgb = GenColor(p, value, t); break;
	case 3: rgb = GenColor(p, q, value); break;
	case 4: rgb = GenColor(t, p, value); break;
	case 5: 
	default:
			rgb = GenColor(value, p, q); break;
	}

	color = (rgb & 0x00ffffff) | (color & 0xff000000);
	return color;
}

D3DXVECTOR4 ColorAccess::ToVec4(const D3DCOLOR& color) {
	return D3DXVECTOR4(GetColorA(color), GetColorR(color), GetColorG(color), GetColorB(color));
}
D3DXVECTOR4 ColorAccess::ToVec4Normalized(const D3DCOLOR& color) {
	D3DXVECTOR4 argb = ToVec4(color);
	__m128 nor = { 255.0f, 255.0f, 255.0f, 255.0f };
	nor = _mm_div_ps((__m128&)argb, nor);
	return (D3DXVECTOR4&)nor;
}
D3DCOLOR ColorAccess::ToD3DCOLOR(const __m128i& color) {
	const int* _i = reinterpret_cast<const int*>(&color);
	return D3DCOLOR_ARGB(_i[0], _i[1], _i[2], _i[3]);
}
D3DCOLOR ColorAccess::ToD3DCOLOR(const D3DXVECTOR4& color) {
	return D3DCOLOR_ARGB((byte)color.x, (byte)color.y, (byte)color.z, (byte)color.w);
}

/**********************************************************
//DxMath
**********************************************************/
bool DxMath::IsIntersected(D3DXVECTOR2& pos, std::vector<D3DXVECTOR2>& list) {
	if (list.size() <= 2) return false;

	bool res = true;
	for (size_t iPos = 0; iPos < list.size(); ++iPos) {
		size_t p1 = iPos;
		size_t p2 = (iPos + 1) % list.size();

		float cross_x = (list[p2].x - list[p1].x) * (pos.y - list[p1].y);
		float cross_y = (list[p2].y - list[p1].y) * (pos.x - list[p1].x);
		if (cross_x - cross_y < 0) res = false;
	}
	return res;
}
bool DxMath::IsIntersected(DxCircle& circle1, DxCircle& circle2) {
	float rx = circle1.GetX() - circle2.GetX();
	float ry = circle1.GetY() - circle2.GetY();
	float rr = circle1.GetR() + circle2.GetR();
	__m128 v1 = { rx, ry, rr, 0 };
	v1 = _mm_mul_ps(v1, v1);
	float* _v1 = reinterpret_cast<float*>(&v1);
	return (_v1[0] + _v1[1]) <= (_v1[2]);
	//return (rx * rx + ry * ry) <= (rr * rr);
}
bool DxMath::IsIntersected(DxCircle& circle, DxWidthLine& line) {
	__m128 v1, v2;
	float* _v1 = reinterpret_cast<float*>(&v1);
	float* _v2 = reinterpret_cast<float*>(&v2);

	{
		v1 = { circle.GetX(), circle.GetY(), circle.GetX(), circle.GetY() };
		v2 = { line.GetX1(), line.GetY1(), line.GetX2(), line.GetY2() };
		v1 = _mm_sub_ps(v1, v2);
		v1 = _mm_mul_ps(v1, v1);

		float dist1 = _v1[0] + _v1[1];
		float dist2 = _v1[2] + _v1[3];

		v2 = { circle.GetR(), dist1, dist2, 0 };
		v2 = _mm_mul_ps(v2, v2);
		if ((_v2[0] >= _v2[1]) || (_v2[0] >= _v2[2]))
			return true;
	}
	{
		v1 = { circle.GetX(), circle.GetY(), circle.GetX(), circle.GetY() };
		v2 = { line.GetX1(), line.GetY1(), line.GetX2(), line.GetY2() };
		v1 = _mm_sub_ps(v1, v2);

		float lx = line.GetX2() - line.GetX1();
		float ly = line.GetY2() - line.GetY1();
		v2 = { lx, ly, -lx, -ly };
		v2 = _mm_mul_ps(v1, v2);

		float inner1 = _v2[0] + _v2[1];
		float inner2 = _v2[2] - _v2[3];
		if (inner1 < 0 || inner2 < 0)
			return false;
	}

	v1 = { line.GetX2(), line.GetY2(), circle.GetX(), circle.GetY() };
	v2 = { line.GetX1(), line.GetY1(), line.GetX1(), line.GetY1() };
	v1 = _mm_sub_ps(v1, v2);

	float ux1 = _v1[0];
	float uy1 = _v1[1];
	float px = _v1[2];
	float py = _v1[3];

	float u = 1.0f / hypotf(ux1, uy1);
	float ux2 = ux1 * u;
	float uy2 = uy1 * u;

	float d = px * ux2 + py * uy2;
	float qx = d * ux2;
	float qy = d * uy2;

	v1 = { px - qx, py - qy, line.GetWidth() + circle.GetR(), 0 };
	v1 = _mm_mul_ps(v1, v1);

	return (_v1[0] + _v1[1]) < _v1[2];
}

//I want to die
bool DxMath::IsIntersected(DxWidthLine& line1, DxWidthLine& line2) {
	auto SplitLines = [](DxWidthLine* pSrcLine, DxWidthLine* pDstLine) -> size_t {
		float dl = hypotf(pSrcLine->GetX2() - pSrcLine->GetX1(), pSrcLine->GetY2() - pSrcLine->GetY1());
		if (dl == 0.0f) return 0U;

		if (abs(pSrcLine->GetWidth()) <= 1.0) {
			*pDstLine = *pSrcLine;
			return 1U;
		}

		float sideScale = pSrcLine->GetWidth() / dl * 0.5f;
		float nx = (pSrcLine->GetX1() - (pSrcLine->GetX1() + pSrcLine->GetX2()) / 2.0f) * sideScale;
		float ny = (pSrcLine->GetY1() - (pSrcLine->GetY1() + pSrcLine->GetY2()) / 2.0f) * sideScale;

		pDstLine[0] = DxWidthLine(pSrcLine->GetX1() + ny, pSrcLine->GetY1() - nx,
			pSrcLine->GetX2() + ny, pSrcLine->GetY2() - nx, 1.0f);
		pDstLine[1] = DxWidthLine(pSrcLine->GetX1() - ny, pSrcLine->GetY1() + nx,
			pSrcLine->GetX2() - ny, pSrcLine->GetY2() + nx, 1.0f);

		return 2U;
	};

	DxWidthLine linePairA[2];
	DxWidthLine linePairB[2];
	size_t countCheckA = SplitLines(&line1, linePairA);
	size_t countCheckB = SplitLines(&line2, linePairB);

	if (countCheckA == 0 || countCheckB == 0) 
		//This ain't a bloody point-point intersection, NEXT!!!
		return false;
	else if (countCheckA == 1 && countCheckB == 1) {
		//A normal line-line intersection

		float dx1 = linePairA->GetX2() - linePairA->GetX1();
		float dy1 = linePairA->GetY2() - linePairA->GetY1();
		float dx2 = linePairB->GetX2() - linePairB->GetX1();
		float dy2 = linePairB->GetY2() - linePairB->GetY1();

		float idet = 1.0f / (dx1 * dy2 - dx2 * dy1);
		float ddx = linePairA->GetX1() - linePairB->GetX1();
		float ddy = linePairA->GetY1() - linePairB->GetY1();
		float s = (dx1 * ddy - dy1 * ddx) * idet;
		float t = (dx2 * ddy - dy2 * ddx) * idet;

		//Issue: fails if the lines are intersecting but parallel
		return (s >= 0 && s <= 1 && t >= 0 && t <= 1);
	}
	else {
		//A polygon-polygon intersection then

		class PolygonIntersect {
		public:
			static const bool IntersectPolygon(DxPoint* listPointA, DxPoint* listPointB, size_t countPointA, size_t countPointB) {
				DxPoint* listPolygon[2] = { listPointA, listPointB };
				size_t listPointCount[2] = { countPointA, countPointB };

				for (size_t iPoly = 0U; iPoly < 2U; ++iPoly) {
					size_t countVert = listPointCount[iPoly];
					for (size_t iPoint = 0U; iPoint < countVert; ++iPoint) {
						DxPoint* p1 = &listPolygon[iPoly][iPoint];
						DxPoint* p2 = &listPolygon[iPoly][(iPoint + 1U) % countVert];
						DxPoint normal(p2->GetY() - p1->GetY(), p1->GetX() - p2->GetX());

						float minA = FLT_MAX;
						float maxA = FLT_MIN;
						for (size_t iPA = 0U; iPA < countPointA; ++iPA) {
							float proj = normal.GetX() * listPointA[iPA].GetX() + normal.GetY() * listPointA[iPA].GetY();
							minA = std::min(minA, proj);
							maxA = std::max(maxA, proj);
						}

						float minB = FLT_MAX;
						float maxB = FLT_MIN;
						for (size_t iPB = 0U; iPB < countPointB; ++iPB) {
							float proj = normal.GetX() * listPointB[iPB].GetX() + normal.GetY() * listPointB[iPB].GetY();
							minB = std::min(minB, proj);
							maxB = std::max(maxB, proj);
						}

						if (maxA < minB || maxB < minA) return false;
					}
				}
				return true;
			}
			static const void FillVertex(DxPoint* pVerts, DxWidthLine* pSrc, size_t countLine) {
				if (countLine == 2U) {
					pVerts[0] = DxPoint(pSrc[0].GetX1(), pSrc[0].GetY1());
					pVerts[1] = DxPoint(pSrc[1].GetX1(), pSrc[1].GetY1());
					pVerts[2] = DxPoint(pSrc[1].GetX2(), pSrc[1].GetY2());
					pVerts[3] = DxPoint(pSrc[0].GetX2(), pSrc[0].GetY2());
				}
				else {
					pVerts[0] = DxPoint(pSrc[0].GetX1(), pSrc[0].GetY1());
					pVerts[1] = DxPoint(pSrc[0].GetX2(), pSrc[0].GetY2());
				}
			};
		};

		DxPoint listPointA[4];
		DxPoint listPointB[4];
		PolygonIntersect::FillVertex(listPointA, linePairA, countCheckA);
		PolygonIntersect::FillVertex(listPointB, linePairB, countCheckB);

		return PolygonIntersect::IntersectPolygon(listPointA, listPointB, countCheckA * 2U, countCheckB * 2U);
	}

	return false;
}

bool DxMath::IsIntersected(DxLine3D& line, std::vector<DxTriangle>& triangles, std::vector<D3DXVECTOR3>& out) {
	out.clear();

	for (size_t iTri = 0; iTri < triangles.size(); ++iTri) {
		DxTriangle& tri = triangles[iTri];
		D3DXPLANE plane;//3ŠpŒ`‚Ì–Ê
		D3DXPlaneFromPoints(&plane, &tri.GetPosition(0), &tri.GetPosition(1), &tri.GetPosition(2));

		D3DXVECTOR3 vOut;// –Ê‚ÆŽ‹ü‚ÌŒð“_‚ÌÀ•W
		if (D3DXPlaneIntersectLine(&vOut, &plane, &line.GetPosition(0), &line.GetPosition(1))) {
			// “àŠO”»’è
			D3DXVECTOR3 vN[3];
			D3DXVECTOR3 vv1, vv2, vv3;
			vv1 = tri.GetPosition(0) - vOut;
			vv2 = tri.GetPosition(1) - vOut;
			vv3 = tri.GetPosition(2) - vOut;
			D3DXVec3Cross(&vN[0], &vv1, &vv3);
			D3DXVec3Cross(&vN[1], &vv2, &vv1);
			D3DXVec3Cross(&vN[2], &vv3, &vv2);
			if (D3DXVec3Dot(&vN[0], &vN[1]) < 0 || D3DXVec3Dot(&vN[0], &vN[2]) < 0)
				continue;
			else {// “à‘¤(3ŠpŒ`‚ÉÚG)
				out.push_back(vOut);
			}
		}
	}//for(int i=0;i<tris.size();i++)

	bool res = out.size() > 0;
	return res;
}

void DxMath::ConstructRotationMatrix(D3DXMATRIX* mat, const D3DXVECTOR2& angleX, 
	const D3DXVECTOR2& angleY, const D3DXVECTOR2& angleZ)
{
	float cx = angleX.x;
	float sx = angleX.y;
	float cy = angleY.x;
	float sy = angleY.y;
	float cz = angleZ.x;
	float sz = angleZ.y;

	float sx_sy = sx * sy;
	float sx_cy = sx * cy;
	__m256 v1_0 = {
		cy, sx_sy, cx, sy, sx_cy, 
		cy, sx_sy, cx
	};
	__m128 v1_1 = { sy, sx_cy, cx, cx };
	__m256 v2_0 = {
		cz, sz, sz, cz, sz,
		sz, cz, cz
	};
	__m128 v2_1 = { sz, cz, sy, cy };
	v1_0 = _mm256_mul_ps(v1_0, v2_0);
	v1_1 = _mm_mul_ps(v1_1, v2_1);
	float* _p0 = reinterpret_cast<float*>(&v1_0);
	float* _p1 = reinterpret_cast<float*>(&v1_1);

	mat->_11 = _p0[0] - _p0[1];
	mat->_12 = -_p0[2];
	mat->_13 = _p0[3] + _p0[4];
	mat->_21 = _p0[5] + _p0[6];
	mat->_22 = _p0[7];
	mat->_23 = _p1[0] - _p1[1];
	mat->_31 = -_p1[2];
	mat->_32 = sx;
	mat->_33 = _p1[3];

	/*
	mat._11 = cy * cz - sx * sy * sz;
	mat._12 = -cx * sz;
	mat._13 = sy * cz + sx * cy * sz;
	mat._21 = cy * sz + sx * sy * cz;
	mat._22 = cx * cz;
	mat._23 = sy * sz - sx * cy * cz;
	mat._31 = -cx * sy;
	mat._32 = sx;
	mat._33 = cx * cy;
	*/
}
void DxMath::MatrixApplyScaling(D3DXMATRIX* mat, const D3DXVECTOR3& scale) {
	__m256 v1 = {
		mat->_11, mat->_12, mat->_13,
		mat->_21, mat->_22, mat->_23,
		mat->_31, mat->_32
	};
	__m256 v2 = {
		scale.x, scale.x, scale.x,
		scale.y, scale.y, scale.y,
		scale.z, scale.z
	};
	v1 = _mm256_mul_ps(v1, v2);
	float* _p = reinterpret_cast<float*>(&v1);
	mat->_11 = _p[0];
	mat->_12 = _p[1];
	mat->_13 = _p[2];
	mat->_21 = _p[3];
	mat->_22 = _p[4];
	mat->_23 = _p[5];
	mat->_31 = _p[6];
	mat->_32 = _p[7];
	mat->_33 = mat->_33 * scale.z;
}
D3DXVECTOR4 DxMath::RotatePosFromXYZFactor(D3DXVECTOR4& vec, D3DXVECTOR2* angX, D3DXVECTOR2* angY, D3DXVECTOR2* angZ) {
	float vx = vec.x;
	float vy = vec.y;
	float vz = vec.z;
	
	if (angZ) {
		float cz = angZ->x;
		float sz = angZ->y;

		vec.x = vx * cz - vy * sz;
		vec.y = vx * sz + vy * cz;
		vx = vec.x;
		vy = vec.y;
	}
	if (angX) {
		float cx = angX->x;
		float sx = angX->y;

		vec.y = vy * cx - vz * sx;
		vec.z = vy * sx + vz * cx;
		vy = vec.y;
		vz = vec.z;
	}
	if (angY) {
		float cy = angY->x;
		float sy = angY->y;

		vec.x = vz * sy + vx * cy;
		vec.z = vz * cy - vx * sy;
	}

	return vec;
}
void DxMath::TransformVertex2D(VERTEX_TLX(&vert)[4], D3DXVECTOR2* scale, D3DXVECTOR2* angle,
	D3DXVECTOR2* position, D3DXVECTOR2* textureSize) 
{
	__m256 v1;
	__m256 v2;
	__m128 v3 = { scale->x, scale->y, scale->x, scale->y };
	float* _p1 = reinterpret_cast<float*>(&v1);

	//First, divide the UVs
	v1 = {
		vert[0].texcoord.x, vert[0].texcoord.y, vert[1].texcoord.x, vert[1].texcoord.y,
		vert[2].texcoord.x, vert[2].texcoord.y, vert[3].texcoord.x, vert[3].texcoord.y
	};
	v2 = {
		textureSize->x, textureSize->y, textureSize->x, textureSize->y,
		textureSize->x, textureSize->y, textureSize->x, textureSize->y
	};
	v1 = _mm256_mul_ps(v1, v2);
	memcpy(&(vert[0].texcoord), &_p1[0], sizeof(D3DXVECTOR2));
	memcpy(&(vert[1].texcoord), &_p1[2], sizeof(D3DXVECTOR2));
	memcpy(&(vert[2].texcoord), &_p1[4], sizeof(D3DXVECTOR2));
	memcpy(&(vert[3].texcoord), &_p1[6], sizeof(D3DXVECTOR2));

	//Then, rotate and scale (2 vertices at a time)
	for (size_t i = 0; i < 4; i += 2) {
		//Rotate
		v1 = {
			vert[i + 0].position.x, vert[i + 0].position.y, vert[i + 0].position.x, vert[i + 0].position.y,
			vert[i + 1].position.x, vert[i + 1].position.y, vert[i + 1].position.x, vert[i + 1].position.y,
		};
		v2 = {
			angle->x, angle->y, angle->y, angle->x,
			angle->x, angle->y, angle->y, angle->x
		};
		v1 = _mm256_mul_ps(v1, v2);

		//Scale
		v2 = {
			_p1[0] - _p1[1], _p1[2] + _p1[3],
			_p1[4] - _p1[5], _p1[6] + _p1[7]
		};
		v1 = {
			position->x, position->y, 
			position->x, position->y,
		};
		v1 = (__m256&)_mm_fmadd_ps((__m128&)v2, v3, (__m128&)v1);

		//Save
		memcpy(&(vert[i + 0].position), &_p1[0], sizeof(D3DXVECTOR2));
		memcpy(&(vert[i + 1].position), &_p1[2], sizeof(D3DXVECTOR2));
	}
}