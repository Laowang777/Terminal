/********************************************************
*                                                       *
*   Copyright (C) Microsoft. All rights reserved.       *
*                                                       *
********************************************************/
#include "precomp.h"

#include "utils.hpp"

#include "..\interactivity\inc\ServiceLocator.hpp"

short CalcWindowSizeX(const SMALL_RECT * const pRect)
{
    return pRect->Right - pRect->Left + 1;
}

short CalcWindowSizeY(const SMALL_RECT * const pRect)
{
    return pRect->Bottom - pRect->Top + 1;
}

short CalcCursorYOffsetInPixels(const short sFontSizeY, const ULONG ulSize)
{
    // TODO: MSFT 10229700 - Note, we want to likely enforce that this isn't negative.
    // Pretty sure there's not a valid case for negative offsets here.
    return (short)((sFontSizeY)-(ulSize));
}

WORD ConvertStringToDec(_In_ PCWSTR pwchToConvert, _Out_opt_ PCWSTR * const ppwchEnd)
{
    WORD val = 0;

    while (*pwchToConvert != L'\0')
    {
        WCHAR ch = *pwchToConvert;
        if (L'0' <= ch && ch <= L'9')
        {
            val = (val * 10) + (ch - L'0');
        }
        else
        {
            break;
        }

        pwchToConvert++;
    }

    if (nullptr != ppwchEnd)
    {
        *ppwchEnd = pwchToConvert;
    }

    return val;
}

// Routine Description:
// - Increments a coordinate relative to the given buffer
// Arguments:
// - bufferSize - Size of the buffer
// - coord - Updated to increment one position, wrapping X and Y dimensions if necessary
void Utils::s_IncrementCoordinate(const COORD bufferSize, COORD& coord)
{
    coord.X++;
    if (coord.X >= bufferSize.X)
    {
        coord.X = 0;
        coord.Y++;

        if (coord.Y >= bufferSize.Y)
        {
            coord.Y = 0;
        }
    }
}

// Routine Description:
// - Decrements a coordinate relative to the given buffer
// Arguments:
// - bufferSize - Size of the buffer
// - coord - Updated to decrement one position, wrapping X and Y dimensions if necessary
void Utils::s_DecrementCoordinate(const COORD bufferSize, COORD& coord)
{
    coord.X--;
    if (coord.X < 0)
    {
        coord.X = bufferSize.X - 1;
        coord.Y--;

        if (coord.Y < 0)
        {
            coord.Y = bufferSize.Y - 1;
        }
    }
}

// Routine Description:
// - Subtracts one from a screen coordinate (in characters, not pixels).
//   Appropriately wraps upward one row when reaching the left edge.
// Arguments:
// - srectEdges - The edges of the screen to use as reference
// - pcoordScreen - Screen coordinate to decrement
// Return Value:
// - True if we could successfully decrement. False if we couldn't (stuck in top left corner).
bool Utils::s_DoDecrementScreenCoordinate(const SMALL_RECT srectEdges, _Inout_ COORD* const pcoordScreen)
{
    // assert that the coord is inside the screen
    FAIL_FAST_IF_FALSE(pcoordScreen->X >= srectEdges.Left);
    FAIL_FAST_IF_FALSE(pcoordScreen->X <= srectEdges.Right);
    FAIL_FAST_IF_FALSE(pcoordScreen->Y >= srectEdges.Top);
    FAIL_FAST_IF_FALSE(pcoordScreen->Y <= srectEdges.Bottom);

    if (pcoordScreen->X == srectEdges.Left)
    {
        if (pcoordScreen->Y > srectEdges.Top)
        {
            pcoordScreen->Y--;
            pcoordScreen->X = srectEdges.Right;
        }
        else
        {
            return false;
        }
    }
    else
    {
        pcoordScreen->X--;
    }

    return true;
}

// Routine Description:
// - Adds one to a screen coordinate (in characters, not pixels).
//   Appropriately wraps downward one row when reaching the right edge.
// Arguments:
// - srectEdges - The edges of the screen to use as reference
// - pcoordScreen - Screen coordinate to increment
// Return Value:
// - True if we could successfully increment. False if we couldn't (stuck in bottom right corner).
bool Utils::s_DoIncrementScreenCoordinate(const SMALL_RECT srectEdges, _Inout_ COORD* const pcoordScreen)
{
    // assert that the coord is inside the screen
    FAIL_FAST_IF_FALSE(pcoordScreen->X >= srectEdges.Left);
    FAIL_FAST_IF_FALSE(pcoordScreen->X <= srectEdges.Right);
    FAIL_FAST_IF_FALSE(pcoordScreen->Y >= srectEdges.Top);
    FAIL_FAST_IF_FALSE(pcoordScreen->Y <= srectEdges.Bottom);

    if (pcoordScreen->X == srectEdges.Right)
    {
        if (pcoordScreen->Y < srectEdges.Bottom)
        {
            pcoordScreen->Y++;
            pcoordScreen->X = srectEdges.Left;
        }
        else
        {
            return false;
        }
    }
    else
    {
        pcoordScreen->X++;
    }

    return true;
}

// Routine Description:
// - Compares two coordinate positions to determine whether they're the same, left, or right within the given buffer size
// Arguments:
// - bufferSize - The size of the buffer to use for measurements.
// - coordFirst - The first coordinate position
// - coordSecond - The second coordinate position
// Return Value:
// -  Negative if First is to the left of the Second.
// -  0 if First and Second are the same coordinate.
// -  Positive if First is to the right of the Second.
// -  This is so you can do s_CompareCoords(first, second) <= 0 for "first is left or the same as second".
//    (the < looks like a left arrow :D)
// -  The magnitude of the result is the distance between the two coordinates when typing characters into the buffer (left to right, top to bottom)
int Utils::s_CompareCoords(const COORD bufferSize, const COORD coordFirst, const COORD coordSecond)
{
    const short cRowWidth = bufferSize.X;

    // Assert that our coordinates are within the expected boundaries
    const short cRowHeight = bufferSize.Y;
    FAIL_FAST_IF_FALSE(coordFirst.X >= 0 && coordFirst.X < cRowWidth);
    FAIL_FAST_IF_FALSE(coordSecond.X >= 0 && coordSecond.X < cRowWidth);
    FAIL_FAST_IF_FALSE(coordFirst.Y >= 0 && coordFirst.Y < cRowHeight);
    FAIL_FAST_IF_FALSE(coordSecond.Y >= 0 && coordSecond.Y < cRowHeight);

    // First set the distance vertically
    //   If first is on row 4 and second is on row 6, first will be -2 rows behind second * an 80 character row would be -160.
    //   For the same row, it'll be 0 rows * 80 character width = 0 difference.
    int retVal = (coordFirst.Y - coordSecond.Y) * cRowWidth;

    // Now adjust for horizontal differences
    //   If first is in position 15 and second is in position 30, first is -15 left in relation to 30.
    retVal += (coordFirst.X - coordSecond.X);

    // Further notes:
    //   If we already moved behind one row, this will help correct for when first is right of second.
    //     For example, with row 4, col 79 and row 5, col 0 as first and second respectively, the distance is -1.
    //     Assume the row width is 80.
    //     Step one will set the retVal as -80 as first is one row behind the second.
    //     Step two will then see that first is 79 - 0 = +79 right of second and add 79
    //     The total is -80 + 79 = -1.
    return retVal;
}

// Routine Description:
// - Compares two coordinate positions to determine whether they're the same, left, or right
// Arguments:
// - coordFirst - The first coordinate position
// - coordSecond - The second coordinate position
// Return Value:
// -  Negative if First is to the left of the Second.
// -  0 if First and Second are the same coordinate.
// -  Positive if First is to the right of the Second.
// -  This is so you can do s_CompareCoords(first, second) <= 0 for "first is left or the same as second".
//    (the < looks like a left arrow :D)
// -  The magnitude of the result is the distance between the two coordinates when typing characters into the buffer (left to right, top to bottom)
int Utils::s_CompareCoords(const COORD coordFirst, const COORD coordSecond)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    // find the width of one row
    const COORD coordScreenBufferSize = gci.GetActiveOutputBuffer().GetScreenBufferSize();
    return s_CompareCoords(coordScreenBufferSize, coordFirst, coordSecond);
}

// Routine Description:
// - Finds the opposite corner given a rectangle and one of its corners.
// - For example, finds the bottom right corner given a rectangle and its top left corner.
// Arguments:
// - srRectangle - The rectangle to check
// - coordCorner - One of the corners of the given rectangle
// - pcoordOpposite - The opposite corner of the one given.
// Return Value:
// - <none>
void Utils::s_GetOppositeCorner(const SMALL_RECT srRectangle, const COORD coordCorner, _Out_ COORD* const pcoordOpposite)
{
    // Assert we were given coordinates that are indeed one of the corners of the rectangle.
    FAIL_FAST_IF_FALSE(coordCorner.X == srRectangle.Left || coordCorner.X == srRectangle.Right);
    FAIL_FAST_IF_FALSE(coordCorner.Y == srRectangle.Top || coordCorner.Y == srRectangle.Bottom);

    pcoordOpposite->X = (srRectangle.Left == coordCorner.X) ? srRectangle.Right : srRectangle.Left;
    pcoordOpposite->Y = (srRectangle.Top == coordCorner.Y) ? srRectangle.Bottom : srRectangle.Top;
}

// Routine Description:
// - Adds the given scalar to the coordinate position given.
//   This needs the edges of the screen buffer as reference to know when
//   to wrap each row.
// Arguments:
// - srectEdges - The edges of the screen to use as reference
// - iAdd - The scalar to add to the coordinate
// - pcoordScreen - Screen coordinate to modify
// Return Value:
// - True if we successfully moved the requested distance. False if we had to stop early.
// - If False, we will restore the original position to the given coordinate.
bool Utils::s_AddToPosition(const SMALL_RECT srectEdges, const int iAdd, _Inout_ COORD* const pcoordPosition)
{
    const COORD coordBackup = *pcoordPosition;
    bool fSuccess = true; // If nothing happens, we're still successful (e.g. iAdd = 0)

    for (int iStart = 0; iStart < iAdd; iStart++)
    {
        fSuccess = s_DoIncrementScreenCoordinate(srectEdges, pcoordPosition);

        // If an operation fails, break.
        if (!fSuccess)
        {
            break;
        }
    }

    for (int iStart = 0; iStart > iAdd; iStart--)
    {
        fSuccess = s_DoDecrementScreenCoordinate(srectEdges, pcoordPosition);

        // If an operation fails, break.
        if (!fSuccess)
        {
            break;
        }
    }

    // If any operation failed, revert to backed up state.
    if (!fSuccess)
    {
        *pcoordPosition = coordBackup;
    }

    return fSuccess;
}

// Routine Description:
// - Retrieves the edges associated with the current screen buffer.
// Arguments:
// - psrectEdges - Pointer to rectangle to fill with edge data.
// Return Value:
// - <none>
void Utils::s_GetCurrentBufferEdges(_Out_ SMALL_RECT* const psrectEdges)
{
    const CONSOLE_INFORMATION& gci = ServiceLocator::LocateGlobals().getConsoleInformation();
    gci.GetActiveOutputBuffer().GetScreenEdges(psrectEdges);
}
