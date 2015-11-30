#include "file_handler.h"

#if WIN32
  #include <cstdint>
  #include <io.h>
  #include <fcntl.h>
  #include <Windows.h>
#else
  #include <stdio.h>
  #include <stdlib.h>
  #include <sys/types.h>
  #include <sys/stat.h>
  #include <unistd.h>
  #include <fcntl.h>
  #include <sys/mman.h>
#endif


const uint32_t file_handler::
GetFirstNodeIdOfDepth(uint32_t depth)
{
    uint32_t id = 0;
    for (uint32_t i = 0; i < depth; ++i)
    {
        id += (uint32_t)pow((double) 2, (double)i);
    }

    return id;
}

const uint32_t file_handler::
GetLengthOfDepth(uint32_t depth)
{
    return (uint32_t)pow((double) 2, (double)depth);
}


file_handler::file_handler(){
  //ctor
  pointsrepository_ = NULL;
  serialtree_ = NULL;
  repositorysize_=0;
  clustersize_=0;
#if !WIN32
  fd_repos = 0;
#endif
}

file_handler::~file_handler(){

#if WIN32
  UnmapViewOfFile(mapped_buffer_);

  CloseHandle(mapped_file_);
#else
  //dtor
  if(fd_repos == 0){
    delete [] pointsrepository_;
    pointsrepository_ = NULL;

  }
  else{
    if (munmap(pointsrepository_, repositorysize_ * sizeof(repositoryPoints)) == -1) {
        perror("Error un-mmapping the file");
    }
    close(fd_repos);
  }
#endif
  delete [] serialtree_;
  serialtree_ = NULL;
}

void
file_handler::load(std::string fileName){
  fileName_ = fileName;
  std::ifstream infile;
  std::string lodfile = fileName + ".lod";
  infile.open(lodfile.c_str(), std::ios::in|std::ios::binary|std::ios::ate); // "ate" = starting at end of file

  if(infile.good()){
  //// read LOD hierarchy ////
    long filesize = infile.tellg(); //get filesize
    long scale = 1; std::string unit = "Byte";
    if(1024 < filesize && filesize < 1048576){
       scale = 1024; unit = "KB";
    }
    else if(filesize>1048576){
      scale = 1048576; unit = "MB";
    }
    std::cout <<"\nloading file: " << lodfile <<". filesize: " <<filesize/scale <<unit;

    infile.seekg(0, std::ios::beg);
    infile.read( reinterpret_cast<char*>(&clustersize_), sizeof(clustersize_) );
    infile.read( reinterpret_cast<char*>(&nodesintree_), sizeof(nodesintree_) );
    infile.read( reinterpret_cast<char*>(&repositorysize_), sizeof(repositorysize_));

    leafescount_ = (nodesintree_>>1)+1;
    treedepth_ = (log(nodesintree_) / log(2)) +1;

    std::cout<<"\nclustersize=" <<clustersize_ <<"\t\t";
    std::cout<<"noodesintree=" <<nodesintree_ <<"\t\t";
    std::cout<<"repositoryelements_=" <<repositorysize_ <<"\n";

    serialtree_ = new serializedTree[nodesintree_];
    infile.read( reinterpret_cast<char*>(serialtree_), (nodesintree_ * sizeof(serialtree_[0])) );

//    std::cout<<"serialTree_[50].idx: " <<serialtree_[50].idx <<"\n";
//    std::cout<<"serialTree_[50].repoidx: " <<serialtree_[50].repoidx <<"\n";
//    std::cout<<"serialTree_[50].poisi: " <<serialtree_[50].poisi <<"\n";
//    std::cout<<"serialTree_[50].clusiz: " <<serialtree_[50].clusiz <<"\n\n";

    pointsrepository_ = new repositoryPoints[repositorysize_];
    infile.read( reinterpret_cast<char*>(pointsrepository_), (repositorysize_ * sizeof(pointsrepository_[0])) );


    if(infile.good()) std::cout <<"loading done." <<std::endl;
    else              std::cout <<"\nError: 0 byte read." <<std::endl;

    infile.close();

//    std::cout<<"pointsrepository_[50].x=" <<pointsrepository_[50].x <<"\t";
//    std::cout<<"pointsrepository_[50].y=" <<pointsrepository_[50].y <<"\t";
//    std::cout<<"pointsrepository_[50].z=" <<pointsrepository_[50].z <<"\n";
//    std::cout<<"pointsrepository_[50].r=" <<(GLint)pointsrepository_[50].r <<"\t";
//    std::cout<<"pointsrepository_[50].g=" <<(GLint)pointsrepository_[50].g <<"\t";
//    std::cout<<"pointsrepository_[50].b=" <<(GLint)pointsrepository_[50].b <<"\n";
//    std::cout<<"pointsrepository_[50].nx=" <<pointsrepository_[50].nx <<"\t";
//    std::cout<<"pointsrepository_[50].ny=" <<pointsrepository_[50].ny <<"\t";
//    std::cout<<"pointsrepository_[50].nz=" <<pointsrepository_[50].nz <<std::endl;

    printBBX(0);

  }
  else{
    std::cout << "\nFailed to open file: " << lodfile <<" ... correct path to file?\n";
    return;
  }
}

void
file_handler::loadMemoryMapped(std::string fileName){
  fileName_ = fileName;
  std::string lodfile = fileName + ".lod";

  std::ifstream lodfile_header; lodfile_header.open((lodfile + "_header").c_str(),std::ios::in|std::ios::binary);
  lodfile_header.read( reinterpret_cast<char*>(&clustersize_), sizeof(clustersize_) );
  lodfile_header.read( reinterpret_cast<char*>(&nodesintree_), sizeof(nodesintree_) );
  lodfile_header.read( reinterpret_cast<char*>(&repositorysize_), sizeof(repositorysize_));

  leafescount_ = (nodesintree_>>1)+1;
  treedepth_ = (log(nodesintree_) / log(2)) +1;

  std::cout<<"\nclustersize=" <<clustersize_ <<"\t\t";
  std::cout<<"noodesintree=" <<nodesintree_ <<"\t\t";
  std::cout<<"repositoryelements_=" <<repositorysize_ <<"\n";

  lodfile_header.close();

  std::ifstream infile; infile.open((lodfile + "_tree").c_str(), std::ios::in|std::ios::binary);
  serialtree_ = new serializedTree[nodesintree_];
  infile.read( reinterpret_cast<char*>(serialtree_), (nodesintree_ * sizeof(serialtree_[0])) );
  infile.close();


#if WIN32

  std::size_t max_size = repositorysize_ * sizeof(repositoryPoints);
  mapped_file_ = CreateFileMapping(
    INVALID_HANDLE_VALUE,    // use paging file
    NULL,                    // default security
    PAGE_READWRITE,          // read/write access
    0,                       // maximum object size (high-order DWORD)
    max_size,                // maximum object size (low-order DWORD)
    (lodfile + "_repos").c_str());                 // name of mapping object

  if (mapped_file_ == NULL)
  {
    std::cerr << "Could not create file mapping object (%d).\n";
  }

  mapped_buffer_ = (LPTSTR)MapViewOfFile(mapped_file_,   // handle to map object
    FILE_MAP_ALL_ACCESS, // read/write permission
    0,
    0,
    max_size);

  if (mapped_buffer_ == NULL)
  {
    std::cerr << "Could not map view of file (%d).\n";
    CloseHandle(mapped_file_);
  }

#else
  fd_repos = open( (lodfile + "_repos").c_str() , O_RDONLY);
  if (fd_repos == -1) {
    perror("Error opening file for reading");
    // return;
  }

  pointsrepository_ = (struct repositoryPoints *) mmap(0, repositorysize_ * sizeof(repositoryPoints), PROT_READ, MAP_PRIVATE /*| MAP_POPULATE*/, fd_repos, 0);
  if (pointsrepository_ == MAP_FAILED) {
    close(fd_repos);
    perror("Error mmapping the file");
    // return;
  }
#endif
#if 0
  for(unsigned long i = 0; i < repositorysize_; ++i){

    std::cout<<pointsrepository_[i].x <<"\n";

  }
#endif

  printBBX(0);

}

void
file_handler::printBBX(unsigned long long nodeid){
  std::cout << "file " << fileName_ << " BBX: "
	    << " min " << serialtree_[nodeid].boundbox.minx << " " << serialtree_[nodeid].boundbox.miny << " " << serialtree_[nodeid].boundbox.minz
	    << " max " << serialtree_[nodeid].boundbox.maxx << " " << serialtree_[nodeid].boundbox.maxy << " " << serialtree_[nodeid].boundbox.maxz
	    << std::endl;
}

void
file_handler::printRepository(void){
  for(long i=0; i<repositorysize_; i++){
    std::cout <<"punkt "<<i <<":\n"  <<pointsrepository_[i].x <<"\t"<<pointsrepository_[i].y<<"\t"<<pointsrepository_[i].z
              <<"\n"<<(long)pointsrepository_[i].r<<"\t"<<(long)pointsrepository_[i].g<<"\t"<<(long)pointsrepository_[i].b
              <<"\n"<<pointsrepository_[i].nx<<"\t"<<pointsrepository_[i].ny<<"\t"<<pointsrepository_[i].nz
              <<std::endl;;
  }
}

long
file_handler::TreeDepth(void){
  return treedepth_;
}

long
file_handler::LeafesCount(void){
  return leafescount_;
}

long
file_handler::Clustersize(void){
  return clustersize_;
}

long
file_handler::NodesInTree(void){
  return nodesintree_;
}

long
file_handler::Repositorysize(void){
  return repositorysize_;
}

serializedTree*
file_handler::SerialTree(void){
  return serialtree_;
}

repositoryPoints*
file_handler::PointsRepository(void){
  return pointsrepository_;
}

