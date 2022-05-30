#include "Segments.h"

#include <algorithm>
#include <cassert>

Segment Segment::operator&(const Segment& Other) const
{
  float NewStart = std::max(Start, Other.Start);
  float NewEnd = std::min(End, Other.End);

  return Segment{ NewStart , NewEnd };
}

bool Segment::IsValid() const
{
  return Start <= End;
}

bool Segment::operator<(const Segment& Other) const
{
  assert(IsValid() && Other.IsValid());
  return End < Other.End;
}

Segment Segment::operator|(const Segment& Other) const
{
  assert(operator&(Other).IsValid());

  float NewStart = std::min(Start, Other.Start);
  float NewEnd = std::max(End, Other.End);

  return Segment{ NewStart, NewEnd };
}

bool Segment::operator==(const Segment& Other) const
{
  return Start == Other.Start && End == Other.End;
}

ArrayType<Segment> Segment::operator-(const Segment& Other) const
{
  if (!Other.IsValid())
  {
    return { *this };
  }

  Segment CommonSegment = operator&(Other);

  ArrayType<Segment> Result;
  if (!CommonSegment.IsValid() || CommonSegment.GetLength() < EPSILON)
  {
    return { *this };
  }

  if (CommonSegment.Start > Start)
  {
    Result.push_back({ Start, CommonSegment.Start });
  }
  if (CommonSegment.End < End)
  {
    Result.push_back({ CommonSegment.End, End });
  }

  return Result;
}

Segment Segment::Invalid()
{ 
  return Segment{ 1, -1 }; 
}

bool SegmentHolder::Contains(Segment Other) const
{
  return (Segments.count(Other) > 0);
}

void SegmentHolder::AddSegment(Segment NewSegment)
{
  const_iterator UnionCandidate = Segments.lower_bound({ NewSegment.Start, NewSegment.Start });
  while (UnionCandidate != Segments.end() && (NewSegment & *UnionCandidate).IsValid())
  {
    NewSegment = NewSegment | *UnionCandidate;
    Segments.erase(UnionCandidate++);
  }

  Segments.insert(NewSegment);
}

void SegmentHolder::RemoveSegment(Segment removal)
{
  const_iterator RemovalCandidate = Segments.upper_bound({ removal.Start, removal.Start });
  while (RemovalCandidate != Segments.end() && (removal & *RemovalCandidate).IsValid())
  {
    auto Difference = (*RemovalCandidate) - removal;
    Segments.erase(RemovalCandidate++);
    for (auto& NewSegment : Difference)
    {
      Segments.insert(NewSegment);
    }
  }
}

SegmentHolder::const_iterator SegmentHolder::begin() const
{
  return Segments.begin();
}

SegmentHolder::const_iterator SegmentHolder::end() const
{
  return Segments.end();
}

SegmentHolder SegmentHolder::operator&(const SegmentHolder& Other) const
{
  SegmentHolder newHolder;

  const_iterator SelfSegment = begin();
  if (SelfSegment == end())
  {
    return newHolder;
  }

  const_iterator OtherSegment = Other.Segments.lower_bound({ SelfSegment->Start,  SelfSegment->Start });

  while (OtherSegment != Other.end() && SelfSegment != end())
  {
    Segment NewSegment = *SelfSegment & *OtherSegment;

    if (NewSegment.IsValid())
    {
      newHolder.AddSegment(NewSegment);
    }

    if (OtherSegment->End > SelfSegment->End)
    {
      SelfSegment++;
    }
    else
    {
      OtherSegment++;
    }
  }

  return newHolder;
}

bool SegmentHolder::operator==(const SegmentHolder& Other) const
{
  return Segments == Other.Segments;
}

SegmentHolder::SegmentHolder()
  : Segments()
{

}

SegmentHolder::SegmentHolder(Segment StartSegment)
  : Segments({ StartSegment })
{

}

void SegmentHolder::LowerSegments(float DeltaTime)
{
  std::set<Segment> NewSegments;

  for (const_iterator Iterator = begin(); Iterator != end(); ++Iterator)
  {
    Segment NewSegment = *Iterator;
    NewSegment.End -= DeltaTime;
    if (NewSegment.IsValid())
    {
      NewSegments.insert(NewSegment);
    }
  }

  Segments = NewSegments;
}


void SegmentHolder::operator-=(float DeltaTime)
{
  std::set<Segment> NewSegments;

  for (const_iterator Iterator = begin(); Iterator != end(); ++Iterator)
  {
    Segment NewSegment = *Iterator;
    NewSegment.End -= DeltaTime;
    NewSegment.Start -= DeltaTime;
    NewSegments.insert(NewSegment);
  }

  Segments = NewSegments;
}

Segment SegmentHolder::Find(float Time) const
{
  auto PossibleSegment = Segments.lower_bound({ Time,  Time });

  if (PossibleSegment == Segments.end())
  {
    return Segment::Invalid();
  }

  return *PossibleSegment;
}
