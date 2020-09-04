#include "gtest/gtest.h"

#include "FileMan.h"

#include "externalized/TestUtils.h"

TEST(FileManTest, joinPaths)
{
	{
		ST::string result;

		// ~~~ platform-specific separators

		result = FileMan::joinPaths("foo", "bar");
		EXPECT_EQ(result, "foo" PATH_SEPARATOR_STR "bar");

		result = FileMan::joinPaths("foo" PATH_SEPARATOR_STR, "bar");
		EXPECT_EQ(result, "foo" PATH_SEPARATOR_STR "bar");

		result = FileMan::joinPaths("foo", PATH_SEPARATOR_STR "bar");
		EXPECT_EQ(result, PATH_SEPARATOR_STR "bar");

		result = FileMan::joinPaths("foo" PATH_SEPARATOR_STR, PATH_SEPARATOR_STR "bar");
		EXPECT_EQ(result, PATH_SEPARATOR_STR "bar");

		// ~~~ generic separators

		result = FileMan::joinPaths("foo", "bar");
		EXPECT_EQ(result, "foo" PATH_SEPARATOR_STR "bar");

		result = FileMan::joinPaths("foo/", "bar");
		EXPECT_EQ(result, "foo/bar");

		result = FileMan::joinPaths("foo", "/bar");
		EXPECT_EQ(result, "/bar");

		result = FileMan::joinPaths("foo/", "/bar");
		EXPECT_EQ(result, "/bar");
	}

}


TEST(FileManTest, joinPathsMultiple)
{
	{
		ST::string result;

		result = FileMan::joinPaths({});
		EXPECT_EQ(result, ST::null);

		result = FileMan::joinPaths({ "foo" });
		EXPECT_EQ(result, "foo");

		result = FileMan::joinPaths({ "foo", "bar" });
		EXPECT_EQ(result, "foo" PATH_SEPARATOR_STR "bar");

		result = FileMan::joinPaths({ "foo", "bar", "baz" });
		EXPECT_EQ(result, "foo" PATH_SEPARATOR_STR "bar" PATH_SEPARATOR_STR "baz");
	}
}


TEST(FileManTest, FindFilesInDir)
{
#define PS PATH_SEPARATOR_STR

	// find one file with .txt estension
	// result on Linux: "unittests/find-files/lowercase-ext.txt"
	// result on Win:   "unittests/find-files\lowercase-ext.txt"

	ST::string testDir = FileMan::joinPaths(GetExtraDataDir(), "unittests/find-files");

	std::vector<ST::string> results = FindFilesInDir(testDir, "txt", false, false);
	ASSERT_EQ(results.size(), 1u);
	EXPECT_STREQ(results[0].c_str(), FileMan::joinPaths(GetExtraDataDir(), "unittests/find-files" PS "lowercase-ext.txt").c_str());

	results = FindFilesInDir(FileMan::joinPaths(GetExtraDataDir(), "unittests" PS "find-files"), "txt", false, false);
	ASSERT_EQ(results.size(), 1u);
	EXPECT_STREQ(results[0].c_str(), FileMan::joinPaths(GetExtraDataDir(), "unittests" PS "find-files" PS "lowercase-ext.txt").c_str());

	results = FindFilesInDir(testDir, "TXT", false, false);
	ASSERT_EQ(results.size(), 1u);
	EXPECT_STREQ(results[0].c_str(), FileMan::joinPaths(GetExtraDataDir(), "unittests/find-files" PS "uppercase-ext.TXT").c_str());

	results = FindFilesInDir(testDir, "TXT", false, true);
	ASSERT_EQ(results.size(), 1u);
	EXPECT_STREQ(results[0].c_str(), "uppercase-ext.TXT");

	results = FindFilesInDir(testDir, "tXt", true, false);
	std::sort(results.begin(), results.end());
	ASSERT_EQ(results.size(), 2u);
	EXPECT_STREQ(results[0].c_str(), FileMan::joinPaths(GetExtraDataDir(), "unittests/find-files" PS "lowercase-ext.txt").c_str());
	EXPECT_STREQ(results[1].c_str(), FileMan::joinPaths(GetExtraDataDir(), "unittests/find-files" PS "uppercase-ext.TXT").c_str());

	results = FindFilesInDir(testDir, "tXt", true, false, true);
	ASSERT_EQ(results.size(), 2u);
	EXPECT_STREQ(results[0].c_str(), FileMan::joinPaths(GetExtraDataDir(), "unittests/find-files" PS "lowercase-ext.txt").c_str());
	EXPECT_STREQ(results[1].c_str(), FileMan::joinPaths(GetExtraDataDir(), "unittests/find-files" PS "uppercase-ext.TXT").c_str());

	results = FindFilesInDir(testDir, "tXt", true, true, true);
	ASSERT_EQ(results.size(), 2u);
	EXPECT_STREQ(results[0].c_str(), "lowercase-ext.txt");
	EXPECT_STREQ(results[1].c_str(), "uppercase-ext.TXT");

	results = FindAllFilesInDir(testDir, true);
	ASSERT_EQ(results.size(), 3u);
	EXPECT_STREQ(results[0].c_str(), FileMan::joinPaths(GetExtraDataDir(), "unittests/find-files" PS "file-without-extension").c_str());
	EXPECT_STREQ(results[1].c_str(), FileMan::joinPaths(GetExtraDataDir(), "unittests/find-files" PS "lowercase-ext.txt").c_str());
	EXPECT_STREQ(results[2].c_str(), FileMan::joinPaths(GetExtraDataDir(), "unittests/find-files" PS "uppercase-ext.TXT").c_str());

#undef PS
}


TEST(FileManTest, RemoveAllFilesInDir)
{
	RustPointer<TempDir> tempDir(TempDir_create());
	ASSERT_NE(tempDir.get(), nullptr);
	RustPointer<char> tempPath(TempDir_path(tempDir.get()));
	ASSERT_NE(tempPath.get(), nullptr);
	ST::string subDir = FileMan::joinPaths(tempPath.get(), "subdir");
	ASSERT_EQ(Fs_createDir(subDir.c_str()), true);

	ST::string pathA = FileMan::joinPaths(tempPath.get(), "foo.txt");
	ST::string pathB = FileMan::joinPaths(tempPath.get(), "bar.txt");

	SGPFile* fileA = FileMan::openForWriting(pathA);
	ASSERT_NE(fileA, nullptr);
	SGPFile* fileB = FileMan::openForWriting(pathB);
	ASSERT_NE(fileB, nullptr);

	FileWrite(fileA, "foo", 3);
	FileWrite(fileB, "bar", 3);

	FileClose(fileA);
	FileClose(fileB);

	std::vector<ST::string> results = FindAllFilesInDir(tempPath.get(), true);
	ASSERT_EQ(results.size(), 2u);

	EraseDirectory(tempPath.get());

	// check that the subdirectory is still there
	ASSERT_EQ(Fs_isDir(subDir.c_str()), true);

	results = FindAllFilesInDir(tempPath.get(), true);
	ASSERT_EQ(results.size(), 0u);
}

TEST(FileManTest, ReadTextFile)
{
	RustPointer<TempDir> tempDir(TempDir_create());
	ASSERT_NE(tempDir.get(), nullptr);
	RustPointer<char> tempPath(TempDir_path(tempDir.get()));
	ASSERT_NE(tempPath.get(), nullptr);
	ST::string pathA = FileMan::joinPaths(tempPath.get(), "foo.txt");

	SGPFile* fileA = FileMan::openForWriting(pathA);
	ASSERT_NE(fileA, nullptr);
	FileWrite(fileA, "foo bar baz", 11);
	FileClose(fileA);

	SGPFile* forReading = FileMan::openForReading(pathA);
	ASSERT_NE(forReading, nullptr);
	ST::string content = FileMan::fileReadText(forReading);
	ASSERT_STREQ(content.c_str(), "foo bar baz");
	FileClose(forReading);
}

TEST(FileManTest, GetFileName)
{
	EXPECT_STREQ(FileMan::getFileName("foo.txt").c_str(),        "foo.txt");
	EXPECT_STREQ(FileMan::getFileName("/a/foo.txt").c_str(),     "foo.txt");
	EXPECT_STREQ(FileMan::getFileName("../a/foo.txt").c_str(),   "foo.txt");

	EXPECT_EQ(FileMan::getFileNameWithoutExt("foo.txt"),       "foo");
	EXPECT_EQ(FileMan::getFileNameWithoutExt("/a/foo.txt"),    "foo");
	EXPECT_EQ(FileMan::getFileNameWithoutExt("../a/foo.txt"),  "foo");
}

#ifdef __WINDOWS__
TEST(FileManTest, GetFileNameWin)
{
	// This tests fail on Linux.
	// Which is might be correct or not correct (depending on your point of view)
	EXPECT_STREQ(FileMan::getFileName("c:\\foo.txt").c_str(),    "foo.txt");
	EXPECT_STREQ(FileMan::getFileName("c:\\b\\foo.txt").c_str(), "foo.txt");

	EXPECT_EQ(FileMan::getFileNameWithoutExt("c:\\foo.txt"),     "foo");
	EXPECT_EQ(FileMan::getFileNameWithoutExt("c:\\b\\foo.txt"),  "foo");
}
#endif

TEST(FileManTest, ReplaceExtension)
{
	EXPECT_EQ(FileMan::replaceExtension("", ""),        "");
	EXPECT_EQ(FileMan::replaceExtension("", "."),       "");
	EXPECT_EQ(FileMan::replaceExtension("", ".bin"),    "");
	EXPECT_EQ(FileMan::replaceExtension("", "bin"),     "");

	EXPECT_EQ(FileMan::replaceExtension("foo.txt", ""),        "foo");
	EXPECT_EQ(FileMan::replaceExtension("foo.txt", "."),       "foo..");
	EXPECT_EQ(FileMan::replaceExtension("foo.txt", ".bin"),    "foo..bin");
	EXPECT_EQ(FileMan::replaceExtension("foo.txt", "bin"),     "foo.bin");

	EXPECT_EQ(FileMan::replaceExtension("foo.bar.txt", ""),        "foo.bar");
	EXPECT_EQ(FileMan::replaceExtension("foo.bar.txt", "."),       "foo.bar..");
	EXPECT_EQ(FileMan::replaceExtension("foo.bar.txt", ".bin"),    "foo.bar..bin");
	EXPECT_EQ(FileMan::replaceExtension("foo.bar.txt", "bin"),     "foo.bar.bin");

	EXPECT_EQ(FileMan::replaceExtension("c:/a/foo.txt", ""),        "c:/a" PATH_SEPARATOR_STR "foo");
	EXPECT_EQ(FileMan::replaceExtension("c:/a/foo.txt", "."),       "c:/a" PATH_SEPARATOR_STR "foo..");
	EXPECT_EQ(FileMan::replaceExtension("c:/a/foo.txt", ".bin"),    "c:/a" PATH_SEPARATOR_STR "foo..bin");
	EXPECT_EQ(FileMan::replaceExtension("c:/a/foo.txt", "bin"),     "c:/a" PATH_SEPARATOR_STR "foo.bin");

	EXPECT_EQ(FileMan::replaceExtension("/a/foo.txt", ""),          "/a" PATH_SEPARATOR_STR "foo");
	EXPECT_EQ(FileMan::replaceExtension("/a/foo.txt", "."),         "/a" PATH_SEPARATOR_STR "foo..");
	EXPECT_EQ(FileMan::replaceExtension("/a/foo.txt", ".bin"),      "/a" PATH_SEPARATOR_STR "foo..bin");
	EXPECT_EQ(FileMan::replaceExtension("/a/foo.txt", "bin"),       "/a" PATH_SEPARATOR_STR "foo.bin");
}

#ifdef __WINDOWS__
TEST(FileManTest, ReplaceExtensionWin)
{
	EXPECT_EQ(FileMan::replaceExtension("c:\\a\\foo.txt", ""),      "c:\\a" PATH_SEPARATOR_STR "foo");
	EXPECT_EQ(FileMan::replaceExtension("c:\\a\\foo.txt", "."),     "c:\\a" PATH_SEPARATOR_STR "foo..");
	EXPECT_EQ(FileMan::replaceExtension("c:\\a\\foo.txt", ".bin"),  "c:\\a" PATH_SEPARATOR_STR "foo..bin");
	EXPECT_EQ(FileMan::replaceExtension("c:\\a\\foo.txt", "bin"),   "c:\\a" PATH_SEPARATOR_STR "foo.bin");
}
#endif

TEST(FileManTest, SlashifyPath)
{
	ST::string test("foo\\bar\\baz");
	FileMan::slashifyPath(test);
	EXPECT_STREQ(test.c_str(), "foo/bar/baz");
}

TEST(FileManTest, FreeSpace)
{
	EXPECT_NE(GetFreeSpaceOnHardDriveWhereGameIsRunningFrom(), 0u);
}
