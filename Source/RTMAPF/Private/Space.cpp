#include "Space.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>

RawSpace::RawSpace(uint32_t InWidth, uint32_t InHeight)
  : Width(InWidth)
  , Height(InHeight)
  , Grid((size_t) InWidth * InHeight, Access::Inaccessable)
{
}

Access RawSpace::GetAccess(FPoint Point) const
{
  return Grid[PointToIndex(Point)];
}

size_t RawSpace::PointToIndex(FPoint& Point) const
{
  size_t Index = Point.X + (size_t) Point.Y * Width;
  assert(Index < Grid.size());
  return Index;
}

void RawSpace::SetAccess(FPoint Point, Access NewAccess)
{
  Grid[PointToIndex(Point)] = NewAccess;
}

uint32_t RawSpace::GetWidth() const
{
  return Width;
}

uint32_t RawSpace::GetHeight() const
{
  return Height;
}

bool RawSpace::Contains(FPoint Point) const
{
  return Point.X >= 0 && (uint32_t) Point.X < Width && Point.Y >= 0 && (uint32_t) Point.Y < Height;
}

SpaceReader::SpaceReader()
  : SymbolToAccess({ 
    {'@', Access::Inaccessable}, 
    {'.', Access::Accessable} })
{
}

TOptional<RawSpace> SpaceReader::FromHogFormat(std::istream& File)
{
  uint32_t Width = 0, Height = 0;

  if (!CheckHogFileStart(File, Width, Height)) return {};
  File.ignore(2, '\n');

  RawSpace ReadSpace(Width, Height);

  char GridValue;
  for (int Row = 0; Row < (int) Height; ++Row)
  {
    for (int Column = 0; Column < (int) Width; ++Column)
    {
      File.get(GridValue);
      Access NewAccess = Access::Inaccessable;
      if (SymbolToAccess.count(GridValue))
      {
        NewAccess = SymbolToAccess[GridValue];
      }

      ReadSpace.SetAccess(FPoint{ Column, Row }, NewAccess);
    }

    File.ignore(2, '\n');
  }

  return std::move(ReadSpace);
}

bool SpaceReader::CheckHogFileStart(std::istream& File, uint32_t& Width, uint32_t& Height)
{
  std::string Buffer;
  std::string Type;

  File >> Buffer;
  if (Buffer != "type")
  {
    std::cerr << "ReadSpace::FromHogFormat: cannot find \"type\" in File\n";
    return false;
  }

  File >> Buffer;
  if (Buffer != "octile")
  {
    std::cerr << "ReadSpace::FromHogFormat: Type is not \"octile\"\n";
    return false;
  }

  File >> Buffer >> Height >> Buffer >> Width >> Buffer;
  if (Buffer != "map")
  {
    std::cerr << "ReadSpace::FromHogFormat: cannot find \"map\" in File\n";
    return false;
  }

  if (File.fail())
  {
    std::cerr << "ReadSpace::FromHogFormat: File read failed\n";
    return false;
  }

  return true;
}

const SegmentHolder& SegmentSpace::GetSegments(FPoint Point) const
{
  assert(ContainsSegmentsIn(Point));
  return SegmentGrid.at(Point);
}

void SegmentSpace::SetSegments(FPoint Point, const SegmentHolder & NewAccess)
{ 
  SegmentGrid[Point] = NewAccess;
}

bool SegmentSpace::ContainsSegmentsIn(FPoint Point) const
{
  return SegmentGrid.count(Point) > 0;
}

void SegmentSpace::SetAccess(const FPoint& Point, Access Access, const float& Depth)
{
  if (SegmentGrid.count(Point) > 0)
  {
    if (Access == Access::Inaccessable)
    {
      SegmentGrid.erase(Point);
    }
  }
  else
  {
    if (Access == Access::Accessable)
    {
      SegmentGrid[Point] = SegmentHolder(Segment{ 0, Depth });
    }
  }
}

SegmentSpace::SegmentSpace(float Depth, const RawSpace& Base)
{
  assert(Depth > 0);

  for (int X = 0; X < (int) Base.GetWidth(); ++X)
  {
    for (int Y = 0; Y < (int) Base.GetHeight(); ++Y)
    {
      FPoint Point = { X, Y };

      if (Base.GetAccess(Point) == Access::Accessable)
      {
        SegmentGrid[Point] = SegmentHolder(Segment{0, Depth});
      }
    }
  }
}

void SegmentSpace::MakeAreasInaccessable(const std::vector<Area>& Areas)
{
  for (const Area& Area : Areas)
  {
    if (!ContainsSegmentsIn(Area.Point))
    {
      continue;
    }
    SegmentGrid[Area.Point].RemoveSegment(Area.Interval);

    // If UsedSegment holder becomes empty, it is still contained inside the SegmentSpace,
    // because in future it may be needed to add accessable intervals there
  }
}

void SegmentSpace::MakeAreasAccessable(const std::vector<Area>& Areas)
{
  for (const Area& Area : Areas)
  {
    if (!ContainsSegmentsIn(Area.Point))
    {
      continue;
    }
    SegmentGrid[Area.Point].AddSegment(Area.Interval);
  }
}

Access SegmentSpace::GetAccess(Area Cell) const
{
  assert(Contains(Cell));

  return SegmentGrid.at(Cell.Point).Contains(Cell.Interval) ? Access::Accessable : Access::Inaccessable;
}

void SegmentSpace::SetAccess(Area Cell, Access Access)
{
  assert(SegmentGrid.count(Cell.Point) > 0);

  if (Access == Access::Accessable)
  {
    SegmentGrid.at(Cell.Point).AddSegment(Cell.Interval);
  }
  else if (Access == Access::Inaccessable)
  {
    SegmentGrid.at(Cell.Point).RemoveSegment(Cell.Interval);
  }
}

bool SegmentSpace::Contains(Area Cell) const
{
  if (SegmentGrid.count(Cell.Point) == 0)
  {
    return false;
  }

  return SegmentGrid.at(Cell.Point).Contains(Cell.Interval);
}

TOptional<Area> SegmentSpace::FindArea(FPoint Point, float Time) const
{
  TOptional<Area> FoundArea;
  for (const auto& UsedSegment : GetSegments(Point))
  {
    if (UsedSegment.Contains(Time))
    {
      FoundArea = { Point, UsedSegment };
      break;
    }
  }
  return FoundArea;
}

SpaceTime::SpaceTime(float InDepth, const RawSpace& Base)
  : SegmentSpace(InDepth, Base)
  , Depth(InDepth)
{

}

float SpaceTime::GetDepth() const
{
  return Depth;
}

SegmentSpace::SegmentSpace()
  : SegmentGrid()
{ }

SpaceTime::SpaceTime(float InDepth)
  : Depth(InDepth)
{ }
