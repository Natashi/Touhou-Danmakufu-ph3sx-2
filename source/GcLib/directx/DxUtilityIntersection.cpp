#include "source/GcLib/pch.h"

#include "DxUtility.hpp"

using namespace gstd;
using namespace directx;

//Never again

#if defined(DNH_PROJ_EXECUTOR)

//*******************************************************************
//DxIntersect
//*******************************************************************
std::vector<DxPoint> DxIntersect::_Polygon_From_LineW(const DxWidthLine* line) {
	std::vector<DxPoint> verts;
	if (abs(line->GetWidth()) < 1.0f) {
		verts.resize(2);
		verts[0] = DxPoint(line->GetX1(), line->GetY1());
		verts[1] = DxPoint(line->GetX2(), line->GetY2());
	}
	else {
		DxLine splitLine[2];
		SplitWidthLine(splitLine, line);

		verts.resize(4);
		verts[0] = DxPoint(splitLine[0].GetX1(), splitLine[0].GetY1());
		verts[1] = DxPoint(splitLine[0].GetX2(), splitLine[0].GetY2());
		verts[2] = DxPoint(splitLine[1].GetX2(), splitLine[1].GetY2());
		verts[3] = DxPoint(splitLine[1].GetX1(), splitLine[1].GetY1());
	}
	return verts;
}
size_t DxIntersect::SplitWidthLine(DxLine* dest, const DxWidthLine* pSrcLine, float mulWidth, bool bForceDouble) {
	float dx = pSrcLine->GetX2() - pSrcLine->GetX1();
	float dy = pSrcLine->GetY2() - pSrcLine->GetY1();

	float dl = hypotf(dx, dy);
	if (dl == 0.0f) return 0U;

	float width = pSrcLine->GetWidth() * mulWidth;

	if (abs(width) <= 1.0f) {
		if (!bForceDouble) {
			dest[0] = *(dynamic_cast<const DxLine*>(pSrcLine));
			return 1U;
		}
		else width = 1.0f;
	}

	__m128 vec1 = Vectorize::Set(pSrcLine->GetX1(), pSrcLine->GetY1(), pSrcLine->GetX2(), pSrcLine->GetY2());
	__m128 vec2 = Vectorize::Mul(
		Vectorize::Set(dy, -dx, dy, -dx),
		Vectorize::Replicate(width / dl * 0.5f));
	{
		__m128 res = Vectorize::Add(vec1, vec2);
		dest[0] = DxLine(res.m128_f32[0], res.m128_f32[1], res.m128_f32[2], res.m128_f32[3]);
		res = Vectorize::Sub(vec1, vec2);
		dest[1] = DxLine(res.m128_f32[0], res.m128_f32[1], res.m128_f32[2], res.m128_f32[3]);
	}

	return 2U;
};

//---------------------------------------------------------------------------------------------------------

bool DxIntersect::Point_Polygon(const DxPoint* pos, const std::vector<DxPoint>* verts) {
	if (verts->size() < 2) return false;

	//https://wrf.ecse.rpi.edu/Research/Short_Notes/pnpoly.html

	float px = pos->GetX();
	float py = pos->GetY();
	size_t nVert = verts->size();
	__m128 v1;

	{
		float minX = FLT_MAX;
		float maxX = FLT_MIN;
		float minY = FLT_MAX;
		float maxY = FLT_MIN;
		for (size_t i = 0; i < nVert; ++i) {
			minX = std::min(minX, (*verts)[i].GetX());
			maxX = std::max(maxX, (*verts)[i].GetX());
			minY = std::min(minY, (*verts)[i].GetY());
			maxY = std::max(maxY, (*verts)[i].GetY());
		}

		//First, check against the bounding box
		if (px < minX || px > maxX || py < minY || py > maxY)
			return false;
	}

	//Fire up the raycasting :(
	bool bInside = false;
	for (size_t i = 0; i < nVert; ++i) {
		size_t j = (i + 1) % nVert;

		float p1x = (*verts)[i].GetX();
		float p1y = (*verts)[i].GetY();
		float p2x = (*verts)[j].GetX();
		float p2y = (*verts)[j].GetY();

		if ((p1y > py) != (p2y > py)) {
			v1 = Vectorize::Sub(
				Vectorize::SetF(p2x, py, p2y, 0),
				Vectorize::SetF(p1x, p1y, p1y, 0));
			if (px < (v1.m128_f32[0] * v1.m128_f32[1] / v1.m128_f32[2] + p1x))
				bInside = !bInside;
		}
	}
	return bInside;
}
bool DxIntersect::Point_Circle(const DxPoint* pos, const DxCircle* circle) {
	float dx = pos->GetX() - circle->GetX();
	float dy = pos->GetY() - circle->GetY();
	__m128 v1 = Vectorize::Mul(
		Vectorize::SetF(dx, dy, circle->GetR(), 0),
		Vectorize::SetF(dx, dy, circle->GetR(), 0));
	return (v1.m128_f32[0] + v1.m128_f32[2]) <= v1.m128_f32[2];
}
bool DxIntersect::Point_Ellipse(const DxPoint* pos, const DxEllipse* ellipse) {
	float dx = pos->GetX() - ellipse->GetX();
	float dy = pos->GetY() - ellipse->GetY();
	__m128 v1 = Vectorize::Mul(
		Vectorize::Set(dx, dy, ellipse->GetA(), ellipse->GetB()),
		Vectorize::Set(dx, dy, ellipse->GetA(), ellipse->GetB()));
	return (v1.m128_f32[0] / v1.m128_f32[2] + v1.m128_f32[1] / v1.m128_f32[3]) <= 1;
}
bool DxIntersect::Point_Line(const DxPoint* pos, const DxLine* line) {
	float dd1 = Math::HypotSq(line->GetX1() - line->GetX2(), line->GetY1() - line->GetY2());
	__m128 v1 = Vectorize::Sub(
		Vectorize::Set(line->GetX1(), line->GetY1(), line->GetX2(), line->GetY2()),
		Vectorize::Set(pos->GetX(), pos->GetY(), pos->GetX(), pos->GetY()));
	float dd2 = v1.m128_f32[0] * v1.m128_f32[1] + v1.m128_f32[2] * v1.m128_f32[3];
	return abs(dd1 - dd2) <= 1;
}
bool DxIntersect::Point_LineW(const DxPoint* pos, const DxWidthLine* line) {
	if (abs(line->GetWidth()) <= 1.0f) {
		return Point_Line(pos, dynamic_cast<const DxLine*>(line));
	}
	std::vector<DxPoint> verts = _Polygon_From_LineW(line);
	return Point_Polygon(pos, &verts);
}
bool DxIntersect::Point_RegularPolygon(const DxPoint* pos, const DxRegularPolygon* polygon) {
	double cpos[2] = { pos->GetX(), pos->GetY() };
	if (polygon->GetAngle() != 0.0f) {
		Math::Rotate2D(cpos, -polygon->GetAngle(), polygon->GetX(), polygon->GetY());
	}

	float f = GM_PI / polygon->GetSide();
	float cf = cosf(f);
	float dx = cpos[0] - polygon->GetX();
	float dy = cpos[1] - polygon->GetY();
	float dist = hypotf(dy, dx);

	bool res = dist <= polygon->GetR();
	if (res) {
		double r_apothem = polygon->GetR() * cf;
		res = dist <= r_apothem;
		if (!res) {
			double ang = fmod(Math::NormalizeAngleRad(atan2(dy, dx)), 2 * f);
			res = dist <= (r_apothem / cos(ang - f));
		}
	}
	return res;
}

bool DxIntersect::Circle_Polygon(const DxCircle* circle, const std::vector<DxPoint>* verts) {
	return Polygon_Circle(verts, circle);
}
bool DxIntersect::Circle_Circle(const DxCircle* circle1, const DxCircle* circle2) {
	__m128 v1 = Vectorize::AddSub(
		Vectorize::Set(circle1->GetX(), circle1->GetR(), circle1->GetY(), 0.0f),
		Vectorize::Set(circle2->GetX(), circle2->GetR(), circle2->GetY(), 0.0f));
	v1 = Vectorize::Mul(v1, v1);
	return (v1.m128_f32[0] + v1.m128_f32[2]) <= (v1.m128_f32[1]);
}
bool DxIntersect::Circle_Ellipse(const DxCircle* circle, const DxEllipse* ellipse) {
	float cx = circle->GetX();
	float cy = circle->GetY();
	float cr = circle->GetR();
	float ex = ellipse->GetX();
	float ey = ellipse->GetY();
	float ea = ellipse->GetA();
	float eb = ellipse->GetB();

	float dx = ex - cx;
	float dy = ey - cy;
	float dx2 = dx * dx;
	float dy2 = dy * dy;

	float cr2 = cr * cr;
	float i_ea2 = 1.0f / (ea * ea);
	float i_eb2 = 1.0f / (eb * eb);

	//One's origin is inside the other
	if (((dx2 + dy2) <= cr2) || ((dx2 * i_ea2 + dy2 * i_eb2) <= 1))
		return true;

	float dd = sqrtf(dx2 + dy2);

	//Eccentricity is negligible, just use a circle-circle intersection
	if (abs(ea - eb) < 0.1)
		return dd <= (cr + ea);

	float th_c = dx / dd;
	float th_s = dy / dd;

	//Point on the ellipse
	float p1_x = ex - ea * th_c;
	float p1_y = ey - eb * th_s;
	if (Math::HypotSq(p1_x - cx, p1_y - cy) <= cr2)
		return true;

	//Point on the circle
	float p2_x = cx + cr * th_c;
	float p2_y = cy + cr * th_s;
	float dx_p2e = p2_x - ex;
	float dy_p2e = p2_y - ey;
	if (((dx_p2e * dx_p2e) * i_ea2 + (dy_p2e * dy_p2e) * i_eb2) <= 1)
		return true;

	return false;
}
bool DxIntersect::Circle_Line(const DxCircle* circle, const DxLine* line) {
	__m128 v1 = Vectorize::Sub(
		Vectorize::Set(circle->GetX(), circle->GetY(), line->GetX2(), line->GetY2()),
		Vectorize::Set(line->GetX1(), line->GetY1(), line->GetX1(), line->GetY1()));
	__m128 v2 = Vectorize::Mul(
		Vectorize::Set(v1.m128_f32[2], v1.m128_f32[3], v1.m128_f32[0], v1.m128_f32[1]),
		Vectorize::Set(v1.m128_f32[2], v1.m128_f32[3], v1.m128_f32[2], v1.m128_f32[3]));
	float dp1 = v2.m128_f32[0] + v2.m128_f32[1];
	float dp2 = v2.m128_f32[2] + v2.m128_f32[3];
	float t = std::clamp<float>(dp2 / dp1, 0, 1);
	float hx = (v1.m128_f32[2] * t + line->GetX1()) - circle->GetX();
	float hy = (v1.m128_f32[3] * t + line->GetY1()) - circle->GetY();
	return Math::HypotSq(hx, hy) <= (circle->GetR() * circle->GetR());
}
bool DxIntersect::Circle_LineW(const DxCircle* circle, const DxWidthLine* line) {
	/*
		A----B	(x1, y1)
		|    |
		|    |
		D----C	(x2, y2)

		<---->	width
	*/
	float cx = circle->GetX();
	float cy = circle->GetY();
	float cr = circle->GetR();
	float x1 = line->GetX1();
	float y1 = line->GetY1();
	float x2 = line->GetX2();
	float y2 = line->GetY2();
	float lw = line->GetWidth();

	//Vectorized / Unvectorized -> ~0.95? (x1.05 times)
#ifdef __L_MATH_VECTORIZE
	__m128 v1, v2;

	float cen_x = (x1 + x2) / 2.0f;
	float cen_y = (y1 + y2) / 2.0f;

	v1 = Vectorize::Sub(
		Vectorize::Set(x2, y2, cen_x, cen_y),
		Vectorize::Set(x1, y1, cx, cy));

	float dx = v1.m128_f32[0];
	float dy = v1.m128_f32[1];
	float u_cx = v1.m128_f32[2];
	float u_cy = v1.m128_f32[3];

	float line_hh = Math::HypotSq(dx, dy);
	if (line_hh < FLT_EPSILON) {
		float dist = Math::HypotSq(cx - x1, cy - y1);
		return dist <= (cr * cr);
	}

	float line_h = sqrtf(line_hh);
	float rcos = dx / line_h;
	float rsin = dy / line_h;

	//Find vector cross products
	v1 = Vectorize::Mul(
		Vectorize::Set(rcos, rsin, rsin, -rcos),
		Vectorize::Set(u_cy, u_cx, u_cy, u_cx));
	float _cross_x = (v1.m128_f32[0] - v1.m128_f32[1]) * 2.0f;
	float _cross_y = (v1.m128_f32[2] - v1.m128_f32[3]) * 2.0f;
	float cross_x = abs(_cross_x);
	float cross_y = abs(_cross_y);

	bool intersect_w = cross_x <= lw;
	bool intersect_h = cross_y <= line_h;
	if (intersect_w && intersect_h)			//The circle's center lies inside the rectangle
		return true;
	else if (intersect_w || intersect_h) {
		//The circle's center lies within these regions
		/*
			|here|
		----A----B----
		here|    |here
		----D----C----
			|here|
		*/

		float r2 = cr * 2.0f;
		if (intersect_w && (cross_y <= line_h + r2))	//Horizontal regions
			return true;
		if (intersect_h && (cross_x <= lw + r2))		//Vertical regions
			return true;
		return false;
	}

	//The circle's center lies within the outer diagonal regions
	/*
	here|    |here
	----A----B----
		|    |
	----D----C----
	here|    |here
	*/

	float l_uw = lw / line_h * 0.5f;
	v1 = Vectorize::Mul(
		Vectorize::Set(dx, dy, cr, 0.0f),
		Vectorize::Set(l_uw, l_uw, cr, 0.0f));
	float nx = v1.m128_f32[0];
	float ny = v1.m128_f32[1];
	float rr = v1.m128_f32[2];

	v1 = Vectorize::Set(cx, cy, cx, cy);
	if (_cross_y > 0) {
		//Check A and B
		v2 = Vectorize::AddSub(
			Vectorize::Set(x1, y1, y1, x1),
			Vectorize::Set(ny, nx, nx, ny));
		v2 = Vectorize::Sub(v1, Vectorize::Set(v2.m128_f32[0], v2.m128_f32[1], v2.m128_f32[3], v2.m128_f32[2]));
		v2 = Vectorize::Mul(v2, v2);
		if (v2.m128_f32[0] + v2.m128_f32[1] <= rr || v2.m128_f32[2] + v2.m128_f32[3] <= rr)
			return true;
	}
	else {
		//Check C and D
		v2 = Vectorize::AddSub(
			Vectorize::Set(y2, x2, x2, y2),
			Vectorize::Set(nx, ny, ny, nx));
		v2 = Vectorize::Sub(v1, Vectorize::Set(v2.m128_f32[1], v2.m128_f32[0], v2.m128_f32[2], v2.m128_f32[3]));
		v2 = Vectorize::Mul(v2, v2);
		if (v2.m128_f32[0] + v2.m128_f32[1] <= rr || v2.m128_f32[2] + v2.m128_f32[3] <= rr)
			return true;
	}
#else
	float rr = cr * cr;
	float cen_x = (x1 + x2) / 2.0f;
	float cen_y = (y1 + y2) / 2.0f;
	float dx = x2 - x1;
	float dy = y2 - y1;
	float line_hh = Math::HypotSq(dx, dy);
	float line_h = sqrtf(line_hh);

	float rcos = dx / line_h;
	float rsin = dy / line_h;

	float cross_x = abs(rcos * (cen_y - cy) - rsin * (cen_x - cx)) * 2.0f;
	float cross_y = abs(rsin * (cen_y - cy) + rcos * (cen_x - cx)) * 2.0f;

	bool intersect_w = cross_x <= lw;
	bool intersect_h = cross_y <= line_h;
	if (intersect_w && intersect_h)
		return true;
	else if (intersect_w || intersect_h) {
		float r2 = cr * 2.0f;
		if (intersect_w && (cross_y <= line_h + r2))
			return true;
		if (intersect_h && (cross_x <= lw + r2))
			return true;
		return false;
	}

	float l_uw = lw / line_h * 0.5f;
	float nx = dx * l_uw;
	float ny = dy * l_uw;

	auto CheckDist = [&](float _tx, float _ty) -> bool {
		return Math::HypotSq(_tx - cx, _ty - cy) <= rr;
	};

	if (CheckDist(x1 - ny, y1 + nx)
		|| CheckDist(x1 + ny, y1 - nx)
		|| CheckDist(x2 + ny, y2 - nx)
		|| CheckDist(x2 - ny, y2 + nx))
		return true;
#endif

	return false;
}
bool DxIntersect::Circle_RegularPolygon(const DxCircle* circle, const DxRegularPolygon* polygon) {
	float cx = circle->GetX();
	float cy = circle->GetY();
	float cr = circle->GetR();
	
	if (cr > 1.0f) {
		float px = polygon->GetX();
		float py = polygon->GetY();
		float pr = polygon->GetR();
		size_t ps = polygon->GetSide();
		float pa = polygon->GetAngle();

		float f = GM_PI_X2 / ps;
		for (size_t i = 0; i < ps; i++, pa += f) {
			float d_pe_cx = (px + pr * cosf(pa)) - cx;
			float d_pe_cy = (py + pr * sinf(pa)) - cy;

			//Check if any of the polygon's vertices are inside the circle
			float dist_ec = hypot(d_pe_cx, d_pe_cy);
			if (dist_ec <= cr)
				return true;

			float theta_c = d_pe_cx / dist_ec;
			float theta_s = d_pe_cy / dist_ec;

			//Check if the point is inside the polygon
			DxPoint tmpPoint(cx + cr * theta_c, cy + cr * theta_s);
			if (Point_RegularPolygon(&tmpPoint, polygon))
				return true;
		}
		return false;
	}
	else {
		return Point_RegularPolygon(dynamic_cast<const DxPoint*>(circle), polygon);
	}
}

bool DxIntersect::Line_Polygon(const DxLine* line, const std::vector<DxPoint>* verts) {
	return Polygon_Line(verts, line);
}
bool DxIntersect::Line_Circle(const DxLine* line, const  DxCircle* circle) {
	return Circle_Line(circle, line);
}
bool DxIntersect::Line_Ellipse(const DxLine* line, const DxEllipse* ellipse) {
	float ex = ellipse->GetX();
	float ey = ellipse->GetY();
	float ea = ellipse->GetA();
	float eb = ellipse->GetB();
	float x1 = line->GetX1() - ex;
	float y1 = line->GetY1() - ey;
	float x2 = line->GetX2() - ex;
	float y2 = line->GetY2() - ey;

	float dlx = x2 - x1;
	float dly = y2 - y1;
	float i_ea2 = 1.0f / (ea * ea);
	float i_eb2 = 1.0f / (eb * eb);

	float A = (dlx * dlx) * i_ea2 + (dly * dly) * i_eb2;
	float B = (2 * x1 * dlx) * i_ea2 + (2 * y1 * dly) * i_eb2;
	float C = (x1 * x1) * i_ea2 + (y1 * y1) * i_eb2 - 1.0f;

	float det = B * B - 4.0f * A * C;
	if (det == 0) {
		float t = -B / (2.0f * A);
		return t >= 0 && t <= 1;
	}
	else if (det > 0) {
		float detsq = sqrtf(det);
		float i_2a = 1.0f / (2.0f * A);
		float t1 = (-B + detsq) * i_2a;
		float t2 = (-B - detsq) * i_2a;
		return (t1 >= 0 && t1 <= 1) || (t2 >= 0 && t2 <= 1);
	}
	return false;
}
bool DxIntersect::Line_Line(const DxLine* line1, const DxLine* line2) {
	{
		//Check bounding box
		DxRect<float> l1_bound = line1->GetBounds();
		DxRect<float> l2_bound = line2->GetBounds();
		if (!l1_bound.IsIntersected(l2_bound))
			return false;
	}
	float s = 0, t = 0;
#ifdef __L_MATH_VECTORIZE
	__m128 v1 = Vectorize::Set(line1->GetX2(), line1->GetY2(), line2->GetX2(), line2->GetY2());
	__m128 v2 = Vectorize::Set(line1->GetX1(), line1->GetY1(), line2->GetX1(), line2->GetY1());

	v1 = Vectorize::Sub(v1, v2);
	v2 = Vectorize::Sub(
		Vectorize::Set(v1.m128_f32[0] * v1.m128_f32[3], line1->GetX1(), line1->GetY1(), 0.0f),
		Vectorize::Set(v1.m128_f32[2] * v1.m128_f32[1], line2->GetX1(), line2->GetY1(), 0.0f));

	float idet = 1.0f / v2.m128_f32[0];
	v2 = Vectorize::Mul(v1,
		Vectorize::Set(v2.m128_f32[2], v2.m128_f32[1], v2.m128_f32[2], v2.m128_f32[1]));
	v2 = Vectorize::Mul(v2, Vectorize::Replicate(idet));
	s = v2.m128_f32[0] - v2.m128_f32[1];
	t = v2.m128_f32[2] - v2.m128_f32[3];
#else
	float dx1 = line1->GetX2() - line1->GetX1();
	float dy1 = line1->GetY2() - line1->GetY1();
	float dx2 = line2->GetX2() - line2->GetX1();
	float dy2 = line2->GetY2() - line2->GetY1();

	float idet = 1.0f / (dx1 * dy2 - dx2 * dy1);
	float ddx = line1->GetX1() - line2->GetX1();
	float ddy = line1->GetY1() - line2->GetY1();
	s = (dx1 * ddy - dy1 * ddx) * idet;
	t = (dx2 * ddy - dy2 * ddx) * idet;
#endif
	//Issue: fails if the lines are intersecting but parallel
	return (s >= 0 && s <= 1 && t >= 0 && t <= 1);
}
bool DxIntersect::Line_LineW(const DxLine* line1, const DxWidthLine* line2) {
	std::vector<DxPoint> verts2 = _Polygon_From_LineW(line2);
	return Polygon_Line(&verts2, line1);
}
bool DxIntersect::Line_RegularPolygon(const DxLine* line, const DxRegularPolygon* polygon) {
	float pr = polygon->GetR();
	size_t ps = polygon->GetSide();
	float pa = polygon->GetAngle();

	std::vector<DxPoint> tmpVerts;
	tmpVerts.resize(ps);

	float f = GM_PI_X2 / ps;
	for (size_t i = 0; i < ps; i++, pa += f) {
		float p_sx = polygon->GetX() + pr * cosf(pa);
		float p_sy = polygon->GetY() + pr * sinf(pa);
		tmpVerts[i] = DxPoint(p_sx, p_sy);
	}

	return Line_Polygon(line, &tmpVerts);
}

bool DxIntersect::LineW_Polygon(const DxWidthLine* line, const std::vector<DxPoint>* verts) {
	std::vector<DxPoint> verts2 = _Polygon_From_LineW(line);
	return Polygon_Polygon(&verts2, verts);
}
bool DxIntersect::LineW_Circle(const DxWidthLine* line, const DxCircle* circle) {
	std::vector<DxPoint> verts = _Polygon_From_LineW(line);
	return Polygon_Circle(&verts, circle);
}
bool DxIntersect::LineW_Ellipse(const DxWidthLine* line, const DxEllipse* ellipse) {
	std::vector<DxPoint> verts = _Polygon_From_LineW(line);
	return Polygon_Ellipse(&verts, ellipse);
}
bool DxIntersect::LineW_Line(const DxWidthLine* line1, const DxLine* line2) {
	std::vector<DxPoint> verts = _Polygon_From_LineW(line1);
	return Polygon_Line(&verts, line2);
}
bool DxIntersect::LineW_LineW(const DxWidthLine* line1, const DxWidthLine* line2) {
	float wd1 = line1->GetWidth();
	float wd2 = line2->GetWidth();
	bool bValid1 = abs(wd1) > 1.0f;
	bool bValid2 = abs(wd2) > 1.0f;
	if (bValid1 && bValid2) {
		std::vector<DxPoint> verts1 = _Polygon_From_LineW(line1);
		std::vector<DxPoint> verts2 = _Polygon_From_LineW(line2);
		return Polygon_Polygon(&verts1, &verts2);
	}
	else if (bValid1) {		//line2 is a simple DxLine
		return LineW_Line(line1, dynamic_cast<const DxLine*>(line2));
	}
	else if (bValid2) {		//line1 is a simple DxLine
		return LineW_Line(line2, dynamic_cast<const DxLine*>(line1));
	}
	else {	//Both are simple DxLines
		return Line_Line(dynamic_cast<const DxLine*>(line1), 
			dynamic_cast<const DxLine*>(line2));
	}
	return false;
}
bool DxIntersect::LineW_RegularPolygon(const DxWidthLine* line, const DxRegularPolygon* polygon) {
	std::vector<DxPoint> verts = _Polygon_From_LineW(line);
	return Polygon_RegularPolygon(&verts, polygon);
}

bool DxIntersect::Polygon_Polygon(const std::vector<DxPoint>* verts1, const std::vector<DxPoint>* verts2) {
	size_t vertCount1 = verts1->size();
	size_t vertCount2 = verts2->size();
	if (vertCount1 < 2 || vertCount1 < 2)
		return false;

	const std::vector<DxPoint>* listPolygon[2] = { verts1, verts2 };
	for (size_t iPoly = 0U; iPoly < 2; ++iPoly) {
		const std::vector<DxPoint>* pPolygon = listPolygon[iPoly];
		size_t countVert = pPolygon->size();

		for (size_t iPoint = 0U; iPoint < countVert; ++iPoint) {
			const DxPoint* p1 = &pPolygon->at(iPoint);
			const DxPoint* p2 = &pPolygon->at((iPoint + 1) % countVert);

			float dx = p1->GetX() - p2->GetX();
			float dy = p2->GetY() - p1->GetY();

			float minA = FLT_MAX, maxA = FLT_MIN;
			float minB = FLT_MAX, maxB = FLT_MIN;
			for (size_t iPA = 0U; iPA < vertCount1; ++iPA) {
				float proj = dy * verts1->at(iPA).GetX() + dx * verts1->at(iPA).GetY();
				minA = std::min(minA, proj);
				maxA = std::max(maxA, proj);
			}
			for (size_t iPB = 0U; iPB < vertCount2; ++iPB) {
				float proj = dy * verts2->at(iPB).GetX() + dx * verts2->at(iPB).GetY();
				minB = std::min(minB, proj);
				maxB = std::max(maxB, proj);
			}

			if (maxA < minB || maxB < minA) return false;
		}
	}
	return true;
}
bool DxIntersect::Polygon_Circle(const std::vector<DxPoint>* verts, const DxCircle* circle) {
	if (verts->size() < 2) return false;

	//Check if circle center is inside the polygon
	if (Point_Polygon(dynamic_cast<const DxPoint*>(circle), verts))
		return true;

	for (size_t i = 0; i < verts->size(); ++i) {
		size_t j = (i + 1) % verts->size();

		DxLine tmpLine((*verts)[i].GetX(), (*verts)[i].GetY(), (*verts)[j].GetX(), (*verts)[j].GetY());
		if (Line_Circle(&tmpLine, circle))
			return true;
	}
	return false;
}
bool DxIntersect::Polygon_Ellipse(const std::vector<DxPoint>* verts, const DxEllipse* ellipse) {
	if (verts->size() < 2) return false;

	//Check if ellipse center is inside the polygon
	if (Point_Polygon(dynamic_cast<const DxPoint*>(ellipse), verts))
		return true;

	for (size_t i = 0; i < verts->size(); ++i) {
		size_t j = (i + 1) % verts->size();

		DxLine tmpLine((*verts)[i].GetX(), (*verts)[i].GetY(), (*verts)[j].GetX(), (*verts)[j].GetY());
		if (Line_Ellipse(&tmpLine, ellipse))
			return true;
	}
	return false;
}
bool DxIntersect::Polygon_Line(const std::vector<DxPoint>* verts, const DxLine* line) {
	if (verts->size() < 2) return false;

	//Check if either of the line terminals are inside the polygon
	DxPoint tmpPoint = DxPoint(line->GetX1(), line->GetY1());
	if (Point_Polygon(&tmpPoint, verts))
		return true;
	tmpPoint = DxPoint(line->GetX2(), line->GetY2());
	if (Point_Polygon(&tmpPoint, verts))
		return true;

	for (size_t i = 0; i < verts->size(); ++i) {
		size_t j = (i + 1) % verts->size();

		DxLine tmpLine((*verts)[i].GetX(), (*verts)[i].GetY(), (*verts)[j].GetX(), (*verts)[j].GetY());
		if (Line_Line(line, &tmpLine))
			return true;
	}
	return false;
}
bool DxIntersect::Polygon_LineW(const std::vector<DxPoint>* verts, const DxWidthLine* line) {
	std::vector<DxPoint> verts2 = _Polygon_From_LineW(line);
	return Polygon_Polygon(&verts2, verts);
}
bool DxIntersect::Polygon_RegularPolygon(const std::vector<DxPoint>* verts, const DxRegularPolygon* polygon) {
	float pr = polygon->GetR();
	size_t ps = polygon->GetSide();
	float pa = polygon->GetAngle();

	std::vector<DxPoint> tmpVerts;
	tmpVerts.resize(ps);

	float f = GM_PI_X2 / ps;
	for (size_t i = 0; i < ps; i++, pa += f) {
		float p_sx = polygon->GetX() + pr * cosf(pa);
		float p_sy = polygon->GetY() + pr * sinf(pa);
		tmpVerts[i] = DxPoint(p_sx, p_sy);
	}

	return Polygon_Polygon(verts, &tmpVerts);
}

#endif