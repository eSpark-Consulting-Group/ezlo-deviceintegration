
#include "test_utils/temporary_folder_wrapper.h"

#include "system_utils/filesystem.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <fstream>

/*

TESTPLAN

DONE    1. Filesystem functions
DONE        1.0 Directory existing function
DONE            1.0.1 Existing folder path
DONE            1.0.2 Not existing folder path
DONE            1.0.3 Existing file path
DONE            1.0.4 Existing folder link
DONE            1.0.5 Existing file link
DONE            1.0.6 Not existing link
DONE        1.1 File existing function
DONE            1.1.1 Existing file path
DONE            1.1.2 Not existing file path
DONE            1.1.3 Existing directory path
DONE            1.1.4 Existing folder link
DONE            1.1.5 Existing file link
DONE            1.1.6 Not existing link
DONE        1.2 General existing function
DONE            1.2.1 Existing file path
DONE            1.2.2 Existing directory path
DONE            1.2.3 Not existing path
DONE        1.3 Folder creation
DONE            1.3.1 Create new folder
DONE            1.3.2 Create a few folders
DONE            1.3.3 Create a few folders with duplicated path delimiter
DONE            1.3.4 Create new folder by relative path
DONE            1.3.5 Create a few folders by relative path
DONE            1.3.6 Try to create folder by existing folder path
DONE            1.3.7 Try to create folder by existing file path
DONE            1.3.8 Try to create and clear the folder
DONE            1.3.9 Try to create a random folder
DONE        1.4 Folder removing
DONE            1.4.1 Delete empty folder
DONE            1.4.2 Delete folder with file
DONE            1.4.3 Delete folder with subfolder
DONE            1.4.4 Delete folder with file and subfolder
DONE            1.4.5 Delete folder with hidden subfolder
DONE            1.4.6 Delete folder by relative path
DONE            1.4.7 Try to delete folder by not existing path
DONE        1.5 Folder copying
DONE            1.5.1 Copy empty folder
DONE            1.5.2 Copy folder with file
DONE            1.5.3 Copy folder with subfolder
DONE            1.5.4 Copy folder with files and subfolder with files
DONE            1.5.5 Copy folder with hidden subfolder
DONE            1.5.6 Copy folder by relative path
DONE            1.5.7 Try to Copy folder by not existing path
DONE        1.6 Folder clear
DONE            1.6.1 Clear empty folder
DONE            1.6.2 Clear non-empty folder
DONE        1.7 Find files in folder
DONE            1.7.1 Folder isn't exist
DONE            1.7.2 No files in folder
DONE            1.7.3 Several files in folder
DONE    2 Filesystem size

*/

namespace HubPlatform {
namespace UT {

class FilesystemToolsTest : public ::testing::Test {
public:
    const std::string& getWorkPath() const {
        return m_temporaryFolder.getPath();
    }

    void createFile( const std::string& filePath ) {
        std::ofstream newFile;
        newFile.open( filePath, std::iostream::out );
        newFile << "test";
        newFile.close();
    }

private:
    Tests::TemporaryFolderWrapper m_temporaryFolder;
};

TEST_F( FilesystemToolsTest, isFolderExistIfFolderExist /*1.0.1*/ ) {
    auto const isExist = filesystem::isDirectoryExist( getWorkPath() );
    EXPECT_TRUE( isExist ) << "'" << getWorkPath() << "' folder should be exist!";
}

TEST_F( FilesystemToolsTest, isFolderExistIfFolderNotExist /*1.0.2*/ ) {
    const auto folderPath = getWorkPath() + "/folder";
    auto const isExist = filesystem::isDirectoryExist( folderPath );
    EXPECT_FALSE( isExist ) << "'" << folderPath << "' folder should not be exist!";
}

TEST_F( FilesystemToolsTest, isFolderExistIfItIsFile /*1.0.3*/ ) {
    const auto filePath = getWorkPath() + "/file.txt";

    createFile( filePath );

    auto const isExist = filesystem::isDirectoryExist( filePath );
    EXPECT_FALSE( isExist ) << "'" << filePath << "' is file. It should not be exist!";
}

TEST_F( FilesystemToolsTest, isFolderExistIfItIsDirectoryLink /*1.0.4*/ ) {
    const auto folderLinkPath = getWorkPath() + "/link";

    auto const isCreated = filesystem::createSymbolicLink( getWorkPath(), folderLinkPath );
    EXPECT_TRUE( isCreated ) << "'" << folderLinkPath << "' symbolic link should be created!";

    auto const isExist = filesystem::isDirectoryExist( folderLinkPath );
    EXPECT_TRUE( isExist ) << "'" << folderLinkPath << "' is symbolic link on '" << getWorkPath()
                           << "' . It should be exist!";
}

TEST_F( FilesystemToolsTest, isFolderExistIfItIsFileLink /*1.0.5*/ ) {
    const auto sourceFilePath = getWorkPath() + "/file.txt";
    const auto folderLinkPath = getWorkPath() + "/link";

    createFile( sourceFilePath );

    auto const isCreated = filesystem::createSymbolicLink( sourceFilePath, folderLinkPath );
    EXPECT_TRUE( isCreated ) << "'" << folderLinkPath << "' symbolic link should be created!";

    auto const isExist = filesystem::isDirectoryExist( folderLinkPath );
    EXPECT_FALSE( isExist ) << "'" << folderLinkPath << "' is symbolic link on '" << sourceFilePath
                            << "' file. It should not be exist!";
}

TEST_F( FilesystemToolsTest, isFolderExistIfItIsBrokenDirectoryLink /*1.0.6*/ ) {
    const auto wrongSourcePath = getWorkPath() + "/source";
    const auto folderLinkPath = getWorkPath() + "/link";

    auto const isCreated = filesystem::createSymbolicLink( wrongSourcePath, folderLinkPath );
    EXPECT_TRUE( isCreated ) << "'" << folderLinkPath << "' symbolic link should be created!";

    auto const isExist = filesystem::isDirectoryExist( folderLinkPath );
    EXPECT_FALSE( isExist ) << "'" << folderLinkPath << "' is broken symbolic link on '" << wrongSourcePath
                            << "' . It should not be exist!";
}

TEST_F( FilesystemToolsTest, isFileExistIfFileExist /*1.1.1*/ ) {
    const auto filePath = getWorkPath() + "/file.txt";

    createFile( filePath );
    auto const isExist = filesystem::isFileExist( filePath );
    EXPECT_TRUE( isExist ) << "'" << filePath << "' file should be exist!";
}

TEST_F( FilesystemToolsTest, isFileExistIfFileNotExist /*1.1.2*/ ) {
    const auto filePath = getWorkPath() + "/file.txt";
    auto const isExist = filesystem::isFileExist( filePath );
    EXPECT_FALSE( isExist ) << "'" << filePath << "' file should not be exist!";
}

TEST_F( FilesystemToolsTest, isFileExistIfItIsFolder /*1.1.3*/ ) {
    auto const isExist = filesystem::isFileExist( getWorkPath() );
    EXPECT_FALSE( isExist ) << "'" << getWorkPath() << "' is folder. It should not be exist!";
}

TEST_F( FilesystemToolsTest, isFileExistIfItFolderLink /*1.1.4*/ ) {
    const auto linkPath = getWorkPath() + "/link";

    const auto isLinkCreated = filesystem::createSymbolicLink( getWorkPath(), linkPath );
    EXPECT_TRUE( isLinkCreated ) << "'" << linkPath << "' file should be created!";

    auto const isExist = filesystem::isFileExist( linkPath );
    EXPECT_FALSE( isExist ) << "'" << linkPath << "' is link on directory. It should not be exist!";
}

TEST_F( FilesystemToolsTest, isFileExistIfItFileLink /*1.1.5*/ ) {
    const auto sourceFilePath = getWorkPath() + "/file.txt";
    const auto linkPath = getWorkPath() + "/link";

    createFile( sourceFilePath );

    const auto isLinkCreated = filesystem::createSymbolicLink( sourceFilePath, linkPath );
    EXPECT_TRUE( isLinkCreated ) << "'" << linkPath << "' file should be created!";

    auto const isExist = filesystem::isFileExist( linkPath );
    EXPECT_TRUE( isExist ) << "'" << linkPath << "' is link on file. It should be exist!";
}

TEST_F( FilesystemToolsTest, isFileExistIfItBrokenLink /*1.1.6*/ ) {
    const auto sourceFilePath = getWorkPath() + "/file.txt";
    const auto linkPath = getWorkPath() + "/link";

    const auto isLinkCreated = filesystem::createSymbolicLink( getWorkPath(), linkPath );
    EXPECT_TRUE( isLinkCreated ) << "'" << linkPath << "' file should be created!";

    auto const isExist = filesystem::isFileExist( linkPath );
    EXPECT_FALSE( isExist ) << "'" << linkPath << "' is broken link. It should not be exist!";
}

TEST_F( FilesystemToolsTest, isExistIfItIsFile /*1.2.1*/ ) {
    const auto filePath = getWorkPath() + "/file.txt";

    createFile( filePath );
    auto const isExist = filesystem::isExist( filePath );
    EXPECT_TRUE( isExist ) << "'" << filePath << "' file should be exist!";
}

TEST_F( FilesystemToolsTest, isExistIfItIsFolder /*1.2.2*/ ) {
    auto const isExist = filesystem::isExist( getWorkPath() );
    EXPECT_TRUE( isExist ) << "'" << getWorkPath() << "' should be exist!";
}

TEST_F( FilesystemToolsTest, isExistIfFileNotExist /*1.2.2*/ ) {
    const auto filePath = getWorkPath() + "/file.txt";
    auto const isExist = filesystem::isExist( filePath );
    EXPECT_FALSE( isExist ) << "'" << filePath << "' file should not be exist!";
}

TEST_F( FilesystemToolsTest, createNewFolder /*1.3.1*/ ) {
    const auto folderPath = getWorkPath() + "/folder";

    auto const isFolderCreated = filesystem::createFolder( folderPath );
    EXPECT_TRUE( isFolderCreated ) << "'" << folderPath << "' folder should be created successfully!";

    const auto isExist = filesystem::isDirectoryExist( folderPath );
    EXPECT_TRUE( isExist ) << "'" << folderPath << "' folder should be exist!";
}

TEST_F( FilesystemToolsTest, createAFewFolders /*1.3.2*/ ) {
    const auto folder1Path = getWorkPath() + "/folder1";
    const auto folder2Path = folder1Path + "/folder2";
    const auto folder3Path = folder2Path + "/folder3";

    auto const isFolderCreated = filesystem::createFolder( folder3Path );
    EXPECT_TRUE( isFolderCreated ) << "'" << folder3Path << "' folder should be created successfully!";

    auto isExist = filesystem::isDirectoryExist( folder1Path );
    EXPECT_TRUE( isExist ) << "'" << folder1Path << "' folder should be exist!";

    isExist = filesystem::isDirectoryExist( folder2Path );
    EXPECT_TRUE( isExist ) << "'" << folder2Path << "' folder should be exist!";

    isExist = filesystem::isDirectoryExist( folder3Path );
    EXPECT_TRUE( isExist ) << "'" << folder3Path << "' folder should be exist!";
}

TEST_F( FilesystemToolsTest, createAFewFoldersWithDuplicatedPathDelimiter /*1.3.3*/ ) {
    const auto folder1Path = getWorkPath() + "/////folder1";
    const auto folder2Path = folder1Path + "//folder2";
    const auto folder3Path = folder2Path + "////folder3";

    auto const isFolderCreated = filesystem::createFolder( folder3Path );
    EXPECT_TRUE( isFolderCreated ) << "'" << folder3Path << "' folder should be created successfully!";

    auto isExist = filesystem::isDirectoryExist( folder1Path );
    EXPECT_TRUE( isExist ) << "'" << folder1Path << "' folder should be exist!";

    isExist = filesystem::isDirectoryExist( folder2Path );
    EXPECT_TRUE( isExist ) << "'" << folder2Path << "' folder should be exist!";

    isExist = filesystem::isDirectoryExist( folder3Path );
    EXPECT_TRUE( isExist ) << "'" << folder3Path << "' folder should be exist!";
}

TEST_F( FilesystemToolsTest, createNewFolderByRelativePath /*1.3.4*/ ) {
    chdir( getWorkPath().c_str() );

    const auto folderPath = "folder";
    const auto fullFolderPath = getWorkPath() + "/" + folderPath;

    auto const isFolderCreated = filesystem::createFolder( folderPath );
    EXPECT_TRUE( isFolderCreated ) << "'" << fullFolderPath << "' folder should be created successfully!";

    const auto isExist = filesystem::isDirectoryExist( fullFolderPath );
    EXPECT_TRUE( isExist ) << "'" << fullFolderPath << "' folder should be exist!";
}

TEST_F( FilesystemToolsTest, createAFewFoldersByRelativePath /*1.3.5*/ ) {
    chdir( getWorkPath().c_str() );

    const auto folder1Path = std::string( "folder1" );
    const auto folder2Path = folder1Path + "/folder2";
    const auto folder3Path = folder2Path + "/folder3";

    auto const isFolderCreated = filesystem::createFolder( folder3Path );
    EXPECT_TRUE( isFolderCreated ) << "'" << getWorkPath() << "/" << folder3Path
                                   << "' folder should be created successfully!";

    auto isExist = filesystem::isDirectoryExist( folder1Path );
    EXPECT_TRUE( isExist ) << "'" << getWorkPath() << "/" << folder1Path << "' folder should be exist!";

    isExist = filesystem::isDirectoryExist( folder2Path );
    EXPECT_TRUE( isExist ) << "'" << getWorkPath() << "/" << folder2Path << "' folder should be exist!";

    isExist = filesystem::isDirectoryExist( folder3Path );
    EXPECT_TRUE( isExist ) << "'" << getWorkPath() << "/" << folder3Path << "' folder should be exist!";
}

TEST_F( FilesystemToolsTest, tryToCreateFolderByExistingFolderPath /*1.3.6*/ ) {
    auto const isFolderCreated = filesystem::createFolder( getWorkPath() );
    EXPECT_TRUE( isFolderCreated ) << "'" << getWorkPath() << "' folder should be created successfully!";

    const auto isExist = filesystem::isDirectoryExist( getWorkPath() );
    EXPECT_TRUE( isExist ) << "'" << getWorkPath() << "' folder should be exist!";
}

TEST_F( FilesystemToolsTest, tryToCreateFolderByExistingFilePath /*1.3.7*/ ) {
    const auto filePath = getWorkPath() + "/file.txt";

    createFile( filePath );

    auto const isFolderCreated = filesystem::createFolder( filePath );
    EXPECT_FALSE( isFolderCreated ) << "'" << filePath << "' folder should not be created!";

    const auto isExist = filesystem::isDirectoryExist( filePath );
    EXPECT_FALSE( isExist ) << "'" << filePath << "' folder should not be exist!";
}

TEST_F( FilesystemToolsTest, tryToCreateAndClearFolder /*1.3.8*/ ) {
    const auto folderPath = getWorkPath() + "/folder";
    const auto filePath = folderPath + "/file.txt";

    filesystem::createFolder( folderPath );
    createFile( filePath );

    EXPECT_TRUE( filesystem::isFileExist( filePath ) ) << "'" << filePath << "' file should exist!";

    filesystem::createAndClearFolder( folderPath );
    EXPECT_FALSE( filesystem::isFileExist( filePath ) ) << "'" << filePath << "' file should not exist!";

    filesystem::deleteFolder( folderPath );

    EXPECT_FALSE( filesystem::isDirectoryExist( folderPath ) ) << "'" << folderPath << "' folder should not exist!";
    filesystem::createAndClearFolder( folderPath );
    EXPECT_TRUE( filesystem::isDirectoryExist( folderPath ) ) << "'" << folderPath << "' folder should exist!";
}

TEST_F( FilesystemToolsTest, tryToCreateRandomFolder /*1.3.9*/ ) {
    const auto folderPath = getWorkPath() + "/folder/best";
    std::string randomFolderName;
    std::string randomFolderName2;

    EXPECT_NO_THROW( randomFolderName = filesystem::createRandomFolder( folderPath ) )
        << "'" << folderPath << "' folder should be created!";
    EXPECT_TRUE( filesystem::isDirectoryExist( randomFolderName ) )
        << "'" << randomFolderName << "' folder should exist!";

    EXPECT_NO_THROW( randomFolderName2 = filesystem::createRandomFolder( folderPath ) )
        << "'" << folderPath << "' folder should be created!";
    EXPECT_NE( randomFolderName, randomFolderName2 ) << "folders' names should not be equal!";
    EXPECT_TRUE( filesystem::isDirectoryExist( randomFolderName2 ) )
        << "'" << randomFolderName2 << "' folder should exist!";
}

TEST_F( FilesystemToolsTest, deleteEmptyFolder /*1.4.1*/ ) {
    const auto removingPath = getWorkPath() + "/folder";
    filesystem::createFolder( removingPath );

    auto const isFolderRemoved = filesystem::deleteFolder( removingPath );
    EXPECT_TRUE( isFolderRemoved ) << "'" << removingPath << "' folder should be removed successfully!";

    const auto isExist = filesystem::isDirectoryExist( removingPath );
    EXPECT_FALSE( isExist ) << "'" << removingPath << "' folder should not be exist!";
}

TEST_F( FilesystemToolsTest, deleteFolderWithFile /*1.4.2*/ ) {
    const auto removingPath = getWorkPath() + "/folder";
    const auto removingFilePath = removingPath + "/file.txt";

    createFile( removingFilePath );
    filesystem::createFolder( removingPath );

    auto const isFolderRemoved = filesystem::deleteFolder( removingPath );
    EXPECT_TRUE( isFolderRemoved ) << "'" << removingPath << "' folder should be removed successfully!";

    const auto isExist = filesystem::isDirectoryExist( removingPath );
    EXPECT_FALSE( isExist ) << "'" << removingPath << "' folder should not be exist!";

    const auto isFileExist = filesystem::isFileExist( removingFilePath );
    EXPECT_FALSE( isFileExist ) << "'" << removingFilePath << "' file should not be exist!";
}

TEST_F( FilesystemToolsTest, deleteFolderWithSubfolder /*1.4.3*/ ) {
    const auto removingPath = getWorkPath() + "/folder1";
    const auto removingSubFolderPath = removingPath + "/folder2";

    filesystem::createFolder( removingSubFolderPath );

    auto const isFolderRemoved = filesystem::deleteFolder( removingPath );
    EXPECT_TRUE( isFolderRemoved ) << "'" << removingPath << "' folder should be removed successfully!";

    auto isExist = filesystem::isDirectoryExist( removingPath );
    EXPECT_FALSE( isExist ) << "'" << removingPath << "' folder should not be exist!";

    isExist = filesystem::isDirectoryExist( removingSubFolderPath );
    EXPECT_FALSE( isExist ) << "'" << removingSubFolderPath << "' folder should not be exist!";
}

TEST_F( FilesystemToolsTest, deleteFolderWithFileAndSubfolder /*1.4.4*/ ) {
    const auto removingPath = getWorkPath() + "/folder1";
    const auto removingFilePath = removingPath + "/file.txt";
    const auto removingSubFolderPath = removingPath + "/folder2";

    filesystem::createFolder( removingSubFolderPath );
    createFile( removingFilePath );

    auto const isFolderRemoved = filesystem::deleteFolder( removingPath );
    EXPECT_TRUE( isFolderRemoved ) << "'" << removingPath << "' folder should be removed successfully!";

    auto isExist = filesystem::isDirectoryExist( removingPath );
    EXPECT_FALSE( isExist ) << "'" << removingPath << "' folder should not be exist!";

    isExist = filesystem::isDirectoryExist( removingSubFolderPath );
    EXPECT_FALSE( isExist ) << "'" << removingSubFolderPath << "' folder should not be exist!";

    isExist = filesystem::isFileExist( removingFilePath );
    EXPECT_FALSE( isExist ) << "'" << removingFilePath << "' file should not be exist!";
}

TEST_F( FilesystemToolsTest, deleteFolderWithHiddenSubfolder /*1.4.5*/ ) {
    const auto removingPath = getWorkPath() + "/folder1";
    const auto removingSubFolderPath = removingPath + "/.folder2";

    filesystem::createFolder( removingSubFolderPath );

    auto const isFolderRemoved = filesystem::deleteFolder( removingPath );
    EXPECT_TRUE( isFolderRemoved ) << "'" << removingPath << "' folder should be removed successfully!";

    auto isExist = filesystem::isDirectoryExist( removingPath );
    EXPECT_FALSE( isExist ) << "'" << removingPath << "' folder should not be exist!";

    isExist = filesystem::isDirectoryExist( removingSubFolderPath );
    EXPECT_FALSE( isExist ) << "'" << removingSubFolderPath << "' folder should not be exist!";
}

TEST_F( FilesystemToolsTest, deleteEmptyFolderByRelativePath /*1.4.6*/ ) {
    chdir( getWorkPath().c_str() );

    const auto removingPath = "folder";
    filesystem::createFolder( getWorkPath() + "/" + removingPath );

    auto const isFolderRemoved = filesystem::deleteFolder( removingPath );
    EXPECT_TRUE( isFolderRemoved ) << "'" << getWorkPath() << "/" << removingPath
                                   << "' folder should be removed successfully!";

    const auto isExist = filesystem::isDirectoryExist( removingPath );
    EXPECT_FALSE( isExist ) << "'" << getWorkPath() << "/" << removingPath << "' folder should not be exist!";
}

TEST_F( FilesystemToolsTest, tryToDeleteFolderByNotExistingPath /*1.4.7*/ ) {
    const auto removingPath = getWorkPath() + "/folder";

    auto const isFolderRemoved = filesystem::deleteFolder( removingPath );
    EXPECT_TRUE( isFolderRemoved ) << "'" << removingPath << "' folder should be removed successfully!";

    const auto isExist = filesystem::isDirectoryExist( getWorkPath() );
    EXPECT_TRUE( isExist ) << "'" << getWorkPath() << "' folder should exist!";
}

TEST_F( FilesystemToolsTest, copyEmptyFolder /*1.5.1*/ ) {
    const auto sourcePath = getWorkPath() + "/folder";
    const auto destinationPath = getWorkPath() + "/destination";

    filesystem::createFolder( sourcePath );

    auto const isFolderCopied = filesystem::copyFolder( sourcePath, destinationPath );
    EXPECT_TRUE( isFolderCopied ) << "'" << sourcePath << "' folder should be copied successfully!";

    const auto isExist = filesystem::isDirectoryExist( destinationPath );
    EXPECT_TRUE( isExist ) << "'" << destinationPath << "' folder should be exist!";
}

TEST_F( FilesystemToolsTest, copyFolderWithFile /*1.5.2*/ ) {
    const auto sourcePath = getWorkPath() + "/folder";
    const auto sourceFilePath = sourcePath + "/file.txt";

    const auto destinationPath = getWorkPath() + "/destination";
    const auto destinationFilePath = destinationPath + "/file.txt";

    filesystem::createFolder( sourcePath );
    createFile( sourceFilePath );

    auto const isFolderCopied = filesystem::copyFolder( sourcePath, destinationPath );
    EXPECT_TRUE( isFolderCopied ) << "'" << sourcePath << "' folder should be copied successfully!";

    const auto isExist = filesystem::isDirectoryExist( destinationPath );
    EXPECT_TRUE( isExist ) << "'" << destinationPath << "' folder should not be exist!";

    const auto isFileExist = filesystem::isFileExist( destinationFilePath );
    EXPECT_TRUE( isFileExist ) << "'" << destinationFilePath << "' file should not be exist!";
}

TEST_F( FilesystemToolsTest, copyFolderWithSubfolder /*1.5.3*/ ) {
    const auto sourcePath = getWorkPath() + "/folder1";
    const auto sourceSubFolderPath = sourcePath + "/folder2";

    const auto destinationPath = getWorkPath() + "/destination";
    const auto destinationSubFolderPath = destinationPath + "/folder2";

    filesystem::createFolder( sourceSubFolderPath );

    auto const isFolderCopied = filesystem::copyFolder( sourcePath, destinationPath );
    EXPECT_TRUE( isFolderCopied ) << "'" << sourcePath << "' folder should be copied successfully!";

    auto isExist = filesystem::isDirectoryExist( destinationPath );
    EXPECT_TRUE( isExist ) << "'" << destinationPath << "' folder should be exist!";

    isExist = filesystem::isDirectoryExist( destinationSubFolderPath );
    EXPECT_TRUE( isExist ) << "'" << destinationSubFolderPath << "' folder should be exist!";
}

TEST_F( FilesystemToolsTest, copyFolderWithFilesAndSubfolderWithFiles /*1.5.4*/ ) {
    const auto sourcePath = getWorkPath() + "/folder1";
    const auto sourceFilePath = sourcePath + "/file.txt";
    const auto sourceFilePath1 = sourcePath + "/file1.txt";
    const auto sourceSubFolderPath = sourcePath + "/afolder2";
    const auto sourceFilePath2 = sourcePath + "/afolder2" + "/file2.txt";
    const auto sourceFilePath3 = sourcePath + "/afolder2" + "/file3.txt";
    const auto sourceFilePath4 = sourcePath + "/afolder2" + "/file4.txt";
    const auto sourceFilePath5 = sourcePath + "/afolder2" + "/file5.txt";

    const auto destinationPath = getWorkPath() + "/destination";
    const auto destinationFilePath = destinationPath + "/file.txt";
    const auto destinationFilePath1 = destinationPath + "/file1.txt";
    const auto destinationSubFolderPath = destinationPath + "/afolder2";
    const auto destinationFilePath2 = destinationPath + "/afolder2" + "/file2.txt";
    const auto destinationFilePath3 = destinationPath + "/afolder2" + "/file3.txt";
    const auto destinationFilePath4 = destinationPath + "/afolder2" + "/file4.txt";
    const auto destinationFilePath5 = destinationPath + "/afolder2" + "/file5.txt";

    filesystem::createFolder( sourceSubFolderPath );
    createFile( sourceFilePath );
    createFile( sourceFilePath1 );
    createFile( sourceFilePath2 );
    createFile( sourceFilePath3 );
    createFile( sourceFilePath4 );
    createFile( sourceFilePath5 );

    auto const isFolderCopied = filesystem::copyFolder( sourcePath, destinationPath );
    EXPECT_TRUE( isFolderCopied ) << "'" << sourcePath << "' folder should be copied successfully!";

    auto isExist = filesystem::isDirectoryExist( destinationPath );
    EXPECT_TRUE( isExist ) << "'" << sourcePath << "' folder should be exist!";

    isExist = filesystem::isDirectoryExist( destinationSubFolderPath );
    EXPECT_TRUE( isExist ) << "'" << destinationSubFolderPath << "' folder should be exist!";

    isExist = filesystem::isFileExist( destinationFilePath );
    EXPECT_TRUE( isExist ) << "'" << destinationFilePath << "' file should be exist!";

    isExist = filesystem::isFileExist( destinationFilePath1 );
    EXPECT_TRUE( isExist ) << "'" << destinationFilePath1 << "' file should be exist!";

    isExist = filesystem::isFileExist( destinationFilePath2 );
    EXPECT_TRUE( isExist ) << "'" << destinationFilePath2 << "' file should be exist!";

    isExist = filesystem::isFileExist( destinationFilePath3 );
    EXPECT_TRUE( isExist ) << "'" << destinationFilePath3 << "' file should be exist!";

    isExist = filesystem::isFileExist( destinationFilePath4 );
    EXPECT_TRUE( isExist ) << "'" << destinationFilePath4 << "' file should be exist!";

    isExist = filesystem::isFileExist( destinationFilePath5 );
    EXPECT_TRUE( isExist ) << "'" << destinationFilePath5 << "' file should be exist!";
}

TEST_F( FilesystemToolsTest, copyFolderWithHiddenSubfolder /*1.5.5*/ ) {
    const auto sourcePath = getWorkPath() + "/folder1";
    const auto sourceSubFolderPath = sourcePath + "/.folder2";

    const auto destinationPath = getWorkPath() + "/destination";
    const auto destinationSubFolderPath = destinationPath + "/.folder2";

    filesystem::createFolder( sourceSubFolderPath );

    auto const isFolderCopied = filesystem::copyFolder( sourcePath, destinationPath );
    EXPECT_TRUE( isFolderCopied ) << "'" << sourcePath << "' folder should be copied successfully!";

    auto isExist = filesystem::isDirectoryExist( destinationPath );
    EXPECT_TRUE( isExist ) << "'" << destinationPath << "' folder should be exist!";

    isExist = filesystem::isDirectoryExist( destinationSubFolderPath );
    EXPECT_TRUE( isExist ) << "'" << destinationSubFolderPath << "' folder should be exist!";
}

TEST_F( FilesystemToolsTest, copyEmptyFolderByRelativePath /*1.5.6*/ ) {
    chdir( getWorkPath().c_str() );

    const auto sourcePath = "folder";
    filesystem::createFolder( getWorkPath() + "/" + sourcePath );

    const auto destinationPath = "destination";

    auto const isFolderCopied = filesystem::copyFolder( sourcePath, destinationPath );
    EXPECT_TRUE( isFolderCopied ) << "'" << getWorkPath() << "/" << sourcePath
                                  << "' folder should be copied successfully!";

    const auto isExist = filesystem::isDirectoryExist( destinationPath );
    EXPECT_TRUE( isExist ) << "'" << getWorkPath() << "/" << destinationPath << "' folder should be exist!";
}

TEST_F( FilesystemToolsTest, tryToCopyFolderByNotExistingPath /*1.5.7*/ ) {
    const auto sourcePath = getWorkPath() + "/folder";
    const auto destinationPath = getWorkPath() + "/destination";

    auto const isFolderCopied = filesystem::copyFolder( sourcePath, destinationPath );
    EXPECT_FALSE( isFolderCopied ) << "'" << sourcePath << "' folder should not be copied successfully!";

    const auto isExist = filesystem::isDirectoryExist( sourcePath );
    EXPECT_FALSE( isExist ) << "'" << sourcePath << "' folder should not be exist!";

    const auto isDestinationExist = filesystem::isDirectoryExist( destinationPath );
    EXPECT_FALSE( isDestinationExist ) << "'" << destinationPath << "' folder should not be exist!";
}

TEST_F( FilesystemToolsTest, symLinkTests ) {
    const std::string file = getWorkPath() + "/testFile";
    createFile( file );

    const std::string linkPath = getWorkPath() + "/testLink";
    ::symlink( file.c_str(), linkPath.c_str() );

    const auto isLinkExist = filesystem::isLinkExist( linkPath );
    EXPECT_TRUE( isLinkExist );

    const auto resultLinkPath = filesystem::getSymlinkValue( linkPath );
    EXPECT_STREQ( file.c_str(), resultLinkPath.c_str() );
}

TEST_F( FilesystemToolsTest, iterateDirectory ) {
    const auto folder = getWorkPath() + "/iterateFolder";
    const auto file1 = folder + "/file1";
    const auto file2 = folder + "/file2";

    filesystem::createFolder( folder );
    createFile( file1 );
    createFile( file2 );

    std::vector< std::string > iterateResult;

    EXPECT_TRUE(
        filesystem::iterateFileSystemEntry( folder, [&]( const std::string& basePath, const std::string& entryName ) {
            const auto it = basePath + "/" + entryName;
            iterateResult.push_back( it );
            return true;
        } ) );

    EXPECT_EQ( iterateResult.size(), 2ul );
    EXPECT_THAT( iterateResult, ::testing::Contains( file1 ) );
    EXPECT_THAT( iterateResult, ::testing::Contains( file2 ) );

    EXPECT_TRUE( filesystem::iterateFileSystemEntry(
        file1, []( const std::string& /*basePath*/, const std::string& /*entryName*/ ) { return false; } ) );
}

TEST_F( FilesystemToolsTest, clearEmptyFolder /*1.6.1*/ ) {
    const auto clearingPath = getWorkPath() + "/folder";
    filesystem::createFolder( clearingPath );

    auto const isFolderCleared = filesystem::clearFolder( clearingPath );
    EXPECT_TRUE( isFolderCleared ) << "'" << clearingPath << "' folder should be cleared successfully!";

    const auto isExist = filesystem::isDirectoryExist( clearingPath );
    EXPECT_TRUE( isExist ) << "'" << clearingPath << "' folder should exist!";
}

TEST_F( FilesystemToolsTest, clearNonEmptyFolder /*1.6.2*/ ) {
    const auto clearingPath = getWorkPath() + "/folder";
    const auto removingFilePath = clearingPath + "/file.txt";

    createFile( removingFilePath );
    filesystem::createFolder( clearingPath );

    auto const isFolderCleared = filesystem::clearFolder( clearingPath );
    EXPECT_TRUE( isFolderCleared ) << "'" << clearingPath << "' folder should be cleared successfully!";

    const auto isExist = filesystem::isDirectoryExist( clearingPath );
    EXPECT_TRUE( isExist ) << "'" << clearingPath << "' folder should exist!";

    const auto isFileExist = filesystem::isFileExist( removingFilePath );
    EXPECT_FALSE( isFileExist ) << "'" << removingFilePath << "' file should not be exist!";
}

TEST_F( FilesystemToolsTest, findFilesFolderNotExist /*1.7.1*/ ) {
    const auto folder = getWorkPath() + "/folder";
    EXPECT_THROW( filesystem::findAllFiles( folder ), filesystem::Exception );
}

TEST_F( FilesystemToolsTest, findFilesEmptyFolder /*1.7.2*/ ) {
    const auto folder = getWorkPath();
    const auto files = filesystem::findAllFiles( folder );
    EXPECT_TRUE( files.empty() );
}

TEST_F( FilesystemToolsTest, findFilesSeveralFiles /*1.7.3*/ ) {
    const auto folder = getWorkPath();
    createFile( folder + "/file1.txt" );
    auto files = filesystem::findAllFiles( folder );
    ASSERT_EQ( files.size(), 1 );
    EXPECT_EQ( files[ 0 ], "file1.txt" );

    createFile( folder + "/file2.mp4" );
    createFile( folder + "/file3.mkv" );
    files = filesystem::findAllFiles( folder );
    ASSERT_EQ( files.size(), 3 );

    EXPECT_NE( std::find( files.begin(), files.end(), "file1.txt" ), files.end() );
    EXPECT_NE( std::find( files.begin(), files.end(), "file2.mp4" ), files.end() );
    EXPECT_NE( std::find( files.begin(), files.end(), "file3.mkv" ), files.end() );
}

TEST_F( FilesystemToolsTest, getFilesystemSize ) /* 2 */ {
    try {
        auto size = filesystem::getFilesystemSize( "/home" );
        ASSERT_GT( size, 0 );
    }
    catch( const filesystem::Exception& ex ) {
        FAIL() << "Filesystem error: " << ex.what();
    }
}

TEST_F( FilesystemToolsTest, folderSizeTest /*3*/ ) {
    const auto folderPath = getWorkPath() + "/folder";

    auto const isFolderCreated = filesystem::createFolder( folderPath );
    EXPECT_TRUE( isFolderCreated ) << "'" << folderPath << "' folder should be created successfully!";

    const auto isExist = filesystem::isDirectoryExist( folderPath );
    EXPECT_TRUE( isExist ) << "'" << folderPath << "' folder should be exist!";

    auto size = filesystem::getFolderSize( folderPath );
    ASSERT_EQ( size, 0 );
}

}  // namespace UT
}  // namespace HubPlatform
