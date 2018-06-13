/*
 Output.h : Output a segment stream
*/

#pragma once

namespace NDecFunc
{
	class CSegment;

	bool OutputConcatenated(const std::string Filename, const std::vector<CSegment*>& Segments);
	bool OutputSeparate(const std::string BaseName, const std::vector<CSegment*>& Segments);
	bool OutputLayerExtract(const std::string BaseName, const std::vector<CSegment*>& Segments, const wxString& OutputFilename=wxEmptyString);
	bool OutputBest(const std::string BaseName, const std::vector<CSegment*>& Segments);
};
