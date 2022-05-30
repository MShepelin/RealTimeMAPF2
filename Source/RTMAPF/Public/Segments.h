#pragma once

#include "SearchTypes.h"

/**
 * Segment desribes time from the Start to the End including both points.
 * If Start <= End then segment is valid. 
 * Invalid segments should be considered empty.
 */
struct Segment
{
  // TODO add accessability?

  float Start;
  float End;

  bool IsValid() const;
  static Segment Invalid();

  /**
   * Intersection of two segments.
   */
  Segment operator&(const Segment& Other) const;

  /**
   * If segments intersect returns a united segment.
   * Otherwise this function shouldn't be used (assertion).
   */
  Segment operator|(const Segment& Other) const;

  std::vector<Segment> operator-(const Segment& Other) const;

  bool operator<(const Segment& Other) const;

  bool operator==(const Segment& Other) const;

  float GetLength() const { //TODO Move to cpp
    return End - Start; 
  }

  bool Contains(float Value) const {
    return IsValid() ? (Value <= End && Value >= Start) : false;
  }
};

MAKE_HASHABLE(Segment, Type.Start, Type.End);

class SegmentHolder
{
private:
  SetType<Segment> Segments;
  using const_iterator = SetType<Segment>::const_iterator;

public:
  SegmentHolder();
  SegmentHolder(Segment StartSegment);

  /**
   * If a new segment doesn't intersect with the stored segments
   * the new one will be added. Otherwise, a united segment will be created.
   */
  void AddSegment(Segment NewSegment);
  
  void RemoveSegment(Segment Removal);

  SegmentHolder operator&(const SegmentHolder& Other) const;

  const_iterator begin() const;
  const_iterator end() const;

  bool operator==(const SegmentHolder& Other) const;
  void operator-=(float DeltaTime);

  void LowerSegments(float DeltaTime);

  bool Contains(Segment Other) const;

  Segment Find(float Time) const;
};

struct Area
{
  FPoint Point;
  Segment Interval;

  bool operator==(const Area& Other) const
  {
    return Point == Other.Point && Interval == Other.Interval;
  }

  explicit Area(const FPoint& InPoint)
    : Point(InPoint)
    , Interval()
  { }

  Area(const FPoint& InPoint, const Segment& InInterval)
    : Point(InPoint)
    , Interval(InInterval)
  { }

  Area()
    : Point()
    , Interval()
  { }

  static Area FromDepth(FPoint Point, float Depth)
  {
    return Area(Point, { Depth, Depth });
  }
};

MAKE_HASHABLE(Area, Type.Point, Type.Interval);
