// Scan.h : Scan for UbiSoft format audio in files
//

#pragma once

/**
 * Inherit from this class to define a callback for the scan process. Both
 * member functions return true to continue scanning, and false to stop. The
 * progress function will only be called if the percent finished is different
 * from the previous time it was called.
 */
class ScanCallback
{
public:

	virtual bool progress(int percent)
	{
		// Default implementation does nothing.
		return true;
	}

	virtual bool foundSegment(std::streamoff offset, std::streamsize size) = 0;
};

// List the UbiSoft format audio chunks in the file
bool ScanAndList(std::istream& input, std::streamoff endOffset, ScanCallback& callback);
