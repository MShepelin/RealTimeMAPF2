#pragma once

#include "Misc/Optional.h"
#include "SearchTypes.h"
#include "Segments.h"

#include <iostream>
#include <stdexcept>

template<typename CellType>
class Space
{
public:
  /**
   * If the Space doesn't contain a Cell, this Cell is inaccessable.
   * If the Space contains a Cell, we can change it's access
   * (it can also become inaccessable).
   */
  virtual bool Contains(CellType Cell) const = 0;

  /**
   * We can attempt to get an access of a Cell only if the Space contains it.
   * Otherwise, the behavoir is undefined.
   */
  virtual Access GetAccess(CellType Cell) const = 0;

  /**
   * We can attempt to set an access of a Cell only if the Space contains it.
   * Otherwise, the behavoir is undefined.
   */
  virtual void SetAccess(CellType Cell, Access Access) = 0;

  // In order to improve Space efficiency GetAccess and SetAccess don't throw exceptions.
  // Standard way to check correctness in run-time is to use assert for Debug.

  virtual ~Space() {};
};

class RawSpace : public Space<FPoint>
{
private:
  uint32_t Width;
  uint32_t Height;
  ArrayType<Access> Grid;

private:
  inline size_t PointToIndex(FPoint& Point) const;

public:
  RawSpace() = delete;
  RawSpace(uint32_t InWidth, uint32_t InHeight);

  Access GetAccess(FPoint Point) const override;
  void SetAccess(FPoint Point, Access NewAccess) override;

  uint32_t GetWidth() const;
  uint32_t GetHeight() const;

  bool Contains(FPoint Point) const override;
};

class SegmentSpace : public Space<Area>
{
protected:
  MapType<FPoint, SegmentHolder> SegmentGrid;

public:
  SegmentSpace();
  SegmentSpace(float Depth, const RawSpace& Base);

  void SetSegments(FPoint Point, const SegmentHolder& NewAccess);
  const SegmentHolder& GetSegments(FPoint Point) const;
  bool ContainsSegmentsIn(FPoint Point) const;
  
  virtual Access GetAccess(Area Cell) const override;
  virtual void SetAccess(Area Cell, Access Access) override;
  virtual bool Contains(Area Cell) const override;

  void MakeAreasInaccessable(const ArrayType<Area>& Areas);
  void MakeAreasAccessable(const std::vector<Area>& Areas);

  TOptional<Area> FindArea(FPoint Point, float Time) const;

  void SetAccess(const FPoint& Point, Access Access, const float& Depth);
};

/**
 * Space that holds time segments limited by [0, Depth]
 */
class SpaceTime : public SegmentSpace
{
protected:
  float Depth;

public:
  SpaceTime(float Depth);
  SpaceTime(float Depth, const RawSpace& Base);

  float GetDepth() const;
};

class SpaceReader
{
private:
  MapType<char, Access> SymbolToAccess;

  inline bool CheckHogFileStart(std::istream& File, uint32_t& Width, uint32_t& Height);

public:
  SpaceReader();

  TOptional<RawSpace> FromHogFormat(std::istream& File);
};
