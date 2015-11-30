#ifndef FILEHANDLER_H
#define FILEHANDLER_H

#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>
#include <cmath>
#include <cstdlib>

#if WIN32
  #include <Windows.h>
  #include <cstdint>
  typedef float GLfloat;
  typedef unsigned char GLubyte;
#else
  #include <GL/glew.h>
#endif


struct vec4 {
  double min,max,inFrustum,unused;
};
struct vec3{
  double x,y,z;
};
struct nodeBounds {
  double minx,miny,minz;
  double maxx,maxy,maxz;
};

struct serializedTree{
  long id; vec4 viewstate; nodeBounds boundbox;
  long repoidx; long clusiz; double minsurfelsize;
  double QuantFactor; vec3 KnotenSchwerpunkt;
};


struct repositoryPoints{
  GLfloat x,y,z;
  GLubyte r,g,b,size;
  GLfloat nx,ny,nz;
};


class file_handler{

  public:
    file_handler();
    ~file_handler();

    void load(std::string fileName);
    void loadMemoryMapped(std::string fileName);
    long Clustersize(void);
    long NodesInTree(void);
    long LeafesCount(void);
    long TreeDepth(void);
    long Repositorysize(void);
    void printRepository(void);

    const uint32_t GetFirstNodeIdOfDepth(uint32_t depth);
    const uint32_t GetLengthOfDepth(uint32_t depth);


    serializedTree* SerialTree(void);
    repositoryPoints* PointsRepository(void);

    void printBBX(unsigned long long nodeid = 0);


  private:

    //old lod-builder kdtree datatypes
    long nodesintree_;
    long repositorysize_;
    long clustersize_;

    //maybe changed later to new lod-builder kdtree datatypes
//    unsigned long nodesintree_;
//    unsigned long repositorysize_;
//    unsigned long clustersize_;

    long leafescount_;
    long treedepth_;

    repositoryPoints* pointsrepository_;
    serializedTree* serialtree_;

#if WIN32
    HANDLE mapped_file_;
    LPCTSTR mapped_buffer_;
#else
    int fd_repos;
#endif

 public:
    std::string fileName_;


};

#endif // FILEHANDLER_H

