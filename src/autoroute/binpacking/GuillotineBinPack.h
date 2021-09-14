/** @file GuillotineBinPack.h
	@author Jukka Jylänki

	@brief Implements different bin packer algorithms that use the GUILLOTINE data structure.

	This work is released to Public Domain, do whatever you want with it.
*/
#pragma once

#include <vector>

#include "Rect.h"

namespace rbp {

/** GuillotineBinPack implements different variants of bin packer algorithms that use the GUILLOTINE data structure
	to keep track of the free space of the bin where rectangles may be placed. */
class GuillotineBinPack
{
public:
	/// The initial bin size will be (0,0). Call Init to set the bin size.
	GuillotineBinPack();

	/// Initializes a new bin of the given size.
	GuillotineBinPack(int width, int height);

	/// (Re)initializes the packer to an empty bin of width x height units. Call whenever
	/// you need to restart with a new bin.
	void Init(int width, int height);

	/// Specifies the different choice heuristics that can be used when deciding which of the free subrectangles
	/// to place the to-be-packed rectangle into.
	enum FreeRectChoiceHeuristic
	{
		RectBestAreaFit, ///< -BAF
		RectBestShortSideFit, ///< -BSSF
		RectBestLongSideFit, ///< -BLSF
		RectWorstAreaFit, ///< -WAF
		RectWorstShortSideFit, ///< -WSSF
		RectWorstLongSideFit ///< -WLSF
	};

	/// Specifies the different choice heuristics that can be used when the packer needs to decide whether to
	/// subdivide the remaining free space in horizontal or vertical direction.
	enum GuillotineSplitHeuristic
	{
		SplitShorterLeftoverAxis, ///< -SLAS
		SplitLongerLeftoverAxis, ///< -LLAS
		SplitMinimizeArea, ///< -MINAS, Try to make a single big rectangle at the expense of making the other small.
		SplitMaximizeArea, ///< -MAXAS, Try to make both remaining rectangles as even-sized as possible.
		SplitShorterAxis, ///< -SAS
		SplitLongerAxis ///< -LAS
	};

	/// Inserts a single rectangle into the bin. The packer might rotate the rectangle, in which case the returned
	/// struct will have the width and height values swapped.
	/// @param merge If true, performs free Rectangle Merge procedure after packing the new rectangle. This procedure
	///		tries to defragment the list of disjoint free rectangles to improve packing performance, but also takes up 
	///		some extra time.
	/// @param rectChoice The free rectangle choice heuristic rule to use.
	/// @param splitMethod The free rectangle split heuristic rule to use.
	Rect Insert(int width, int height, bool merge, FreeRectChoiceHeuristic rectChoice, GuillotineSplitHeuristic splitMethod);

	/// Inserts a list of rectangles into the bin.
	/// @param rects The list of rectangles to add. This list will be destroyed in the packing process.
	/// @param merge If true, performs Rectangle Merge operations during the packing process.
	/// @param rectChoice The free rectangle choice heuristic rule to use.
	/// @param splitMethod The free rectangle split heuristic rule to use.
	void Insert(std::vector<RectSize> &rects, bool merge, 
		FreeRectChoiceHeuristic rectChoice, GuillotineSplitHeuristic splitMethod);

// Implements GUILLOTINE-MAXFITTING, an experimental heuristic that's really cool but didn't quite work in practice.
//	void InsertMaxFitting(std::vector<RectSize> &rects, std::vector<Rect> &dst, bool merge, 
//		FreeRectChoiceHeuristic rectChoice, GuillotineSplitHeuristic splitMethod);

	/// Computes the ratio of used/total surface area. 0.00 means no space is yet used, 1.00 means the whole bin is used.
	float Occupancy() const;

	/// Returns the internal list of disjoint rectangles that track the free area of the bin. You may alter this vector
	/// any way desired, as long as the end result still is a list of disjoint rectangles.
	std::vector<Rect> &GetFreeRectangles() { return freeRectangles; }

	/// Returns the list of packed rectangles. You may alter this vector at will, for example, you can move a Rect from
	/// this list to the Free Rectangles list to free up space on-the-fly, but notice that this causes fragmentation.
	std::vector<Rect> &GetUsedRectangles() { return usedRectangles; }

	/// Performs a Rectangle Merge operation. This procedure looks for adjacent free rectangles and merges them if they
	/// can be represented with a single rectangle. Takes up Theta(|freeRectangles|^2) time.
	void MergeFreeList();

private:
	int binWidth;
	int binHeight;

	/// Stores a list of all the rectangles that we have packed so far. This is used only to compute the Occupancy ratio,
	/// so if you want to have the packer consume less memory, this can be removed.
	std::vector<Rect> usedRectangles;

	/// Stores a list of rectangles that represents the free area of the bin. This rectangles in this list are disjoint.
	std::vector<Rect> freeRectangles;

#ifdef _DEBUG
	/// Used to track that the packer produces proper packings.
	DisjointRectCollection disjointRects;
#endif

	/// Goes through the list of free rectangles and finds the best one to place a rectangle of given size into.
	/// Running time is Theta(|freeRectangles|).
	/// @param nodeIndex [out] The index of the free rectangle in the freeRectangles array into which the new
	///		rect was placed.
	/// @return A Rect structure that represents the placement of the new rect into the best free rectangle.
	Rect FindPositionForNewNode(int width, int height, FreeRectChoiceHeuristic rectChoice, int *nodeIndex);

	static int ScoreByHeuristic(int width, int height, const Rect &freeRect, FreeRectChoiceHeuristic rectChoice);
	// The following functions compute (penalty) score values if a rect of the given size was placed into the 
	// given free rectangle. In these score values, smaller is better.

	static int ScoreBestAreaFit(int width, int height, const Rect &freeRect);
	static int ScoreBestShortSideFit(int width, int height, const Rect &freeRect);
	static int ScoreBestLongSideFit(int width, int height, const Rect &freeRect);

	static int ScoreWorstAreaFit(int width, int height, const Rect &freeRect);
	static int ScoreWorstShortSideFit(int width, int height, const Rect &freeRect);
	static int ScoreWorstLongSideFit(int width, int height, const Rect &freeRect);

	/// Splits the given L-shaped free rectangle into two new free rectangles after placedRect has been placed into it.
	/// Determines the split axis by using the given heuristic.
	void SplitFreeRectByHeuristic(const Rect &freeRect, const Rect &placedRect, GuillotineSplitHeuristic method);

	/// Splits the given L-shaped free rectangle into two new free rectangles along the given fixed split axis.
	void SplitFreeRectAlongAxis(const Rect &freeRect, const Rect &placedRect, bool splitHorizontal);
};

}
