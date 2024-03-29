// This file was generated by qlalr - DO NOT EDIT!
#include "svgpathgrammar_p.h"

const char *const SVGPathGrammar::spell [] = {
	"end of file", nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
	nullptr, nullptr, nullptr, nullptr, nullptr,
#ifndef QLALR_NO_SVGPATHGRAMMAR_DEBUG_INFO
	"path_data", "moveto_drawto_command_groups", "moveto_drawto_command_group", "moveto", "drawto_commands",
	"drawto_command", "fakeclosepath", "closepath", "lineto", "horizontal_lineto", "vertical_lineto", "curveto", "smooth_curveto", "quadratic_bezier_curveto", "smooth_quadratic_bezier_curveto",
	"elliptical_arc", "moveto_command", "moveto_argument_sequence", "coordinate_pair", "comma_wsp", "lineto_command", "lineto_argument_sequence", "horizontal_lineto_command", "horizontal_lineto_argument_sequence", "coordinate",
	"vertical_lineto_command", "vertical_lineto_argument_sequence", "curveto_command", "curveto_argument_sequence", "curveto_argument", "smooth_curveto_command", "smooth_curveto_argument_sequence", "smooth_curveto_argument", "quadratic_bezier_curveto_command", "quadratic_bezier_curveto_argument_sequence",
	"quadratic_bezier_curveto_argument", "elliptical_arc_command", "elliptical_arc_argument_sequence", "elliptical_arc_argument", "nonnegative_number", "number", "flag", "smooth_quadratic_bezier_curveto_command", "smooth_quadratic_bezier_curveto_argument_sequence", "x_coordinate",
	"y_coordinate", "wspplus", "$accept"
#endif // QLALR_NO_SVGPATHGRAMMAR_DEBUG_INFO
};

const int SVGPathGrammar::lhs [] = {
	15, 16, 16, 17, 17, 19, 19, 20, 20, 20,
	20, 20, 20, 20, 20, 20, 20, 18, 32, 32,
	32, 23, 36, 36, 36, 24, 38, 38, 38, 25,
	41, 41, 41, 26, 43, 43, 43, 44, 44, 44,
	44, 27, 46, 46, 46, 47, 47, 28, 49, 49,
	49, 50, 50, 30, 52, 52, 52, 53, 29, 58,
	58, 58, 33, 33, 59, 60, 34, 34, 61, 39,
	54, 55, 56, 31, 35, 37, 40, 42, 45, 48,
	57, 51, 22, 21, 62
};

const int SVGPathGrammar:: rhs[] = {
	1, 1, 2, 2, 1, 1, 2, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 2, 1, 3,
	2, 2, 1, 3, 2, 2, 1, 3, 2, 2,
	1, 3, 2, 2, 1, 2, 3, 5, 4, 4,
	3, 2, 1, 2, 3, 2, 3, 2, 1, 2,
	3, 3, 2, 2, 1, 2, 3, 11, 2, 1,
	3, 2, 3, 2, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 2
};


#ifndef QLALR_NO_SVGPATHGRAMMAR_DEBUG_INFO
const int SVGPathGrammar::rule_info [] = {
	15, 16
	, 16, 17
	, 16, 17, 16
	, 17, 18, 19
	, 17, 18
	, 19, 20
	, 19, 20, 19
	, 20, 21
	, 20, 22
	, 20, 23
	, 20, 24
	, 20, 25
	, 20, 26
	, 20, 27
	, 20, 28
	, 20, 29
	, 20, 30
	, 18, 31, 32
	, 32, 33
	, 32, 33, 34, 32
	, 32, 33, 32
	, 23, 35, 36
	, 36, 33
	, 36, 33, 34, 36
	, 36, 33, 36
	, 24, 37, 38
	, 38, 39
	, 38, 39, 34, 38
	, 38, 39, 38
	, 25, 40, 41
	, 41, 39
	, 41, 39, 34, 41
	, 41, 39, 41
	, 26, 42, 43
	, 43, 44
	, 43, 44, 43
	, 43, 44, 34, 43
	, 44, 33, 34, 33, 34, 33
	, 44, 33, 34, 33, 33
	, 44, 33, 33, 34, 33
	, 44, 33, 33, 33
	, 27, 45, 46
	, 46, 47
	, 46, 47, 46
	, 46, 47, 34, 46
	, 47, 33, 33
	, 47, 33, 34, 33
	, 28, 48, 49
	, 49, 50
	, 49, 50, 49
	, 49, 50, 34, 49
	, 50, 33, 34, 33
	, 50, 33, 33
	, 30, 51, 52
	, 52, 53
	, 52, 53, 52
	, 52, 53, 34, 52
	, 53, 54, 34, 54, 34, 55, 34, 56, 34, 56, 34, 33
	, 29, 57, 58
	, 58, 33
	, 58, 33, 34, 58
	, 58, 33, 58
	, 33, 59, 34, 60
	, 33, 59, 60
	, 59, 39
	, 60, 39
	, 34, 61
	, 34, 12
	, 61, 14
	, 39, 13
	, 54, 13
	, 55, 13
	, 56, 13
	, 31, 1
	, 35, 4
	, 37, 5
	, 40, 6
	, 42, 7
	, 45, 8
	, 48, 9
	, 57, 10
	, 51, 11
	, 22, 2
	, 21, 3
	, 62, 15, 0
};

const int SVGPathGrammar::rule_index [] = {
	0, 2, 4, 7, 10, 12, 14, 17, 19, 21,
	23, 25, 27, 29, 31, 33, 35, 37, 40, 42,
	46, 49, 52, 54, 58, 61, 64, 66, 70, 73,
	76, 78, 82, 85, 88, 90, 93, 97, 103, 108,
	113, 117, 120, 122, 125, 129, 132, 136, 139, 141,
	144, 148, 152, 155, 158, 160, 163, 167, 179, 182,
	184, 188, 191, 195, 198, 200, 202, 204, 206, 208,
	210, 212, 214, 216, 218, 220, 222, 224, 226, 228,
	230, 232, 234, 236, 238
};
#endif // QLALR_NO_SVGPATHGRAMMAR_DEBUG_INFO

const int SVGPathGrammar::action_default [] = {
	0, 74, 5, 0, 2, 1, 0, 82, 76, 78,
	84, 75, 79, 80, 81, 77, 83, 9, 13, 0,
	6, 4, 17, 0, 8, 11, 0, 10, 0, 15,
	0, 14, 0, 16, 0, 12, 0, 70, 65, 0,
	35, 34, 0, 68, 69, 0, 0, 67, 0, 0,
	39, 38, 0, 41, 40, 0, 36, 37, 0, 66,
	64, 63, 7, 71, 55, 54, 0, 0, 56, 57,
	0, 0, 0, 72, 0, 0, 73, 0, 0, 0,
	0, 58, 27, 26, 0, 29, 28, 23, 22, 0,
	25, 24, 0, 49, 48, 0, 53, 52, 0, 50,
	51, 0, 43, 42, 0, 46, 47, 0, 44, 45,
	60, 59, 0, 62, 61, 31, 30, 0, 33, 32,
	19, 18, 0, 21, 20, 3, 85
};

const int SVGPathGrammar::goto_default [] = {
	6, 5, 4, 2, 21, 20, 24, 17, 27, 25,
	35, 18, 31, 29, 33, 22, 3, 121, 39, 45,
	28, 88, 26, 83, 38, 36, 116, 19, 41, 40,
	32, 103, 102, 30, 94, 93, 23, 65, 64, 66,
	74, 77, 34, 111, 42, 60, 47, 0
};

const int SVGPathGrammar::action_index [] = {
	16, -15, 29, -13, 0, -15, 2, -15, -15, -15,
	-15, -15, -15, -15, -15, -15, -15, -15, -15, -13,
	29, -15, -15, -9, -15, -15, -1, -15, -5, -15,
	-3, -15, -7, -15, -1, -15, -1, -15, -15, 10,
	10, -15, 10, -15, -15, -13, 10, -15, 10, -1,
	-15, -15, -1, -15, -15, -1, -15, -15, -1, -15,
	-15, -15, -15, -15, 13, -15, 4, -2, -15, -15,
	-6, 7, -4, -15, 7, -8, -15, 7, 1, 7,
	-1, -15, 10, -15, -1, -15, -15, 10, -15, -13,
	-15, -15, 10, 10, -15, -13, -15, -15, -13, -15,
	-15, 10, 10, -15, -13, -15, -15, -13, -15, -15,
	10, -15, -10, -15, -15, 10, -15, -13, -15, -15,
	10, -15, -13, -15, -15, -15, -15,

	-48, -48, -48, -12, 43, -48, -48, -48, -48, -48,
	-48, -48, -48, -48, -48, -48, -48, -48, -48, -48,
	-4, -48, -48, -48, -48, -48, -5, -48, 6, -48,
	3, -48, -13, -48, -9, -48, -21, -48, -48, -6,
	20, -48, 16, -48, -48, -3, 13, -48, 19, -8,
	-48, -48, -7, -48, -48, -14, -48, -48, 10, -48,
	-48, -48, -48, -48, 8, -48, -11, -30, -48, -48,
	-26, 1, -48, -48, 4, -48, -48, -15, -25, -2,
	0, -48, 50, -48, 18, -48, -48, 42, -48, 15,
	-48, -48, 33, 38, -48, -16, -48, -48, 25, -48,
	-48, 28, 46, -48, -17, -48, -48, 31, -48, -48,
	35, -48, 7, -48, -48, 72, -48, 2, -48, -48,
	49, -48, 12, -48, -48, -48, -48
};

const int SVGPathGrammar::action_info [] = {
	37, 1, 126, 37, 63, 76, 37, 63, 37, 73,
	37, 63, 37, 0, 76, 0, 43, 1, 44, 43,
	0, 44, 43, 37, 44, 43, 63, 44, 0, 0,
	0, 16, 10, 11, 8, 15, 9, 12, 13, 14,
	7, 0, 0, 0,

	62, 106, 97, 115, 78, 101, 120, 69, 70, 110,
	51, 54, 46, 71, 57, 48, 79, 80, 81, 82,
	72, 92, 0, 75, 87, 110, 115, 67, 119, 124,
	120, 53, 52, 87, 59, 58, 91, 50, 49, 55,
	59, 86, 82, 92, 125, 68, 105, 104, 56, 101,
	114, 96, 95, 110, 112, 61, 92, 98, 0, 100,
	87, 89, 109, 90, 101, 107, 123, 120, 122, 84,
	0, 0, 99, 85, 82, 0, 0, 108, 113, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 117, 0, 0, 0, 0, 115, 0, 118, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

const int SVGPathGrammar::action_check [] = {
	13, 1, 0, 13, 13, 13, 13, 13, 13, 13,
	13, 13, 13, -1, 13, -1, 12, 1, 14, 12,
	-1, 14, 12, 13, 14, 12, 13, 14, -1, -1,
	-1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
	11, -1, -1, -1,

	4, 18, 18, 24, 19, 18, 18, 37, 19, 18,
	18, 18, 18, 39, 28, 18, 41, 19, 18, 24,
	19, 18, -1, 19, 18, 18, 24, 19, 26, 17,
	18, 18, 19, 18, 24, 19, 21, 18, 19, 19,
	24, 23, 24, 18, 1, 37, 18, 19, 28, 18,
	43, 18, 19, 18, 19, 45, 18, 19, -1, 34,
	18, 19, 31, 21, 18, 19, 17, 18, 19, 19,
	-1, -1, 34, 23, 24, -1, -1, 31, 43, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, 19, -1, -1, -1, -1, 24, -1, 26, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};
