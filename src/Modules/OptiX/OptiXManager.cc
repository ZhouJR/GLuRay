// #include <optixu/optixpp_namespace.h>
// #include <optixu/optixu_math_namespace.h>
// #include <optixu/optixu_aabb_namespace.h>
// #include <sutil.h>
// #include <GLUTDisplay.h>
// #include <PlyLoader.h>
// #include <ObjLoader.h>
// #include "commonStructs.h"
// #include <string>
// #include <iostream>
// #include <fstream>
// #include <cstdlib>
// #include <cstring>
// #include "random.h"
// #include "MeshScene.h"




#include <UseMPI.h>
#ifdef USE_MPI
#include <Engine/Display/NullDisplay.h>
#include <Engine/LoadBalancers/MPI_LoadBalancer.h>
#include <Engine/ImageTraversers/MPI_ImageTraverser.h>
#include <mpi.h>
#endif

#include "defines.h"
#include "OptiXManager.h"
//#include <X11/Xlib.h>
//#include <X11/Xutil.h>

//#include "../gl_functions.h"
#include "CDTimer.h"
#include "OScene.h"
#include "ORenderable.h"
#include "common.h"
#include <Modules/Manta/AccelWork.h>
#include <OBJScene.h>

#include <Core/Util/Logger.h>

#include <Model/Primitives/KenslerShirleyTriangle.h>
#include <Engine/PixelSamplers/RegularSampler.h>
#include <Engine/Display/FileDisplay.h>
#include <Image/SimpleImage.h>
#include <Model/Materials/Dielectric.h>
#include <Model/Materials/ThinDielectric.h>
#include <Model/Materials/OrenNayar.h>
#include <Model/Materials/Transparent.h>
#include <Model/AmbientLights/AmbientOcclusionBackground.h>
#include <Model/AmbientLights/AmbientOcclusion.h>
#include <Model/Textures/TexCoordTexture.h>
#include <Model/Textures/Constant.h>
#include <Model/Primitives/Plane.h>
#include <Model/Primitives/Parallelogram.h>
#include <Model/Primitives/Cube.h>
#include <Model/Primitives/Disk.h>



#include <stdio.h>
#include <cassert>
#include <float.h>
#include <stack>
#include <algorithm>

#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>



#include <arpa/inet.h>

#include <GL/gl.h>

OptiXManager* OptiXManager::_singleton = NULL;

OptiXManager* OptiXManager::singleton()
{
  if (!_singleton)
    _singleton = new OptiXManager();
  return _singleton;
}


OptiXManager::OptiXManager()
:RenderManager(), current_scene(NULL), next_scene(NULL),
_nid_counter(0), _depth(false), _width(0), _height(0), _frameNumber(0), _realFrameNumber(0)
{
  initialized=false;
  printf("%s::%s\n",typeid(*this).name(),__FUNCTION__);
  _format = "RGBA8";

  _gVoid = new OGeometryGeneratorVoid();
  _gTriangle = new OGeometryGeneratorTriangles();
  _gTriangleStrip = new OGeometryGeneratorTriangleStrip();
  _gQuads = new OGeometryGeneratorQuads();
  _gQuadStrip = new OGeometryGeneratorQuadStrip();
  _gLines = new OGeometryGeneratorLines();
  _gLineStrip= new OGeometryGeneratorLineStrip();

}

OptiXManager::~OptiXManager()
{
}

void OptiXManager::updateLights()
{

}

Renderable* OptiXManager::createRenderable(GeometryGenerator* gen)
{
  OGeometryGenerator* mg = dynamic_cast<OGeometryGenerator*>(gen);
  assert(mg);
  return new ORenderable(mg);
}

void  OptiXManager::updateMaterial()
{
  if (!initialized)
    return;
  GLMaterial m = gl_material;
  //TODO: DEBUG: hardcoding mat for debugging
  // m.diffuse = Color(RGB(1.0, 0.713726, .21569));
  //m.diffuse = Color(RGB(0.8, 0.8, 0.8));
  // m.specular = Manta::Color(RGB(.1, .1, .1));
  // m.ambient = Manta::Color(RGB(0, 0, 0));
  // m.shiny = 100;

}

void OptiXManager::useShadows(bool st)
{
}

void OptiXManager::setSize(int w, int h)
{

  printf("setSize %d %d\n", w,h);
  if (initialized && (w != _width || h != _height))
  {
    _width = w; _height = h;
    updateCamera();
  }
}


void OptiXManager::init()
{
  if (initialized)
    return;
  initialized=true;
  printf("%s::%s\n",typeid(*this).name(),__FUNCTION__);

  updateBackground();
  updateCamera();
  updateMaterial();
  if (!current_scene)
    current_scene = new OScene();
  if (!next_scene)
    next_scene = new OScene();

}

//TODO: updating pixelsampler mid flight crashes manta
void OptiXManager::setNumSamples(int,int,int samples)
{
}

void OptiXManager::setNumThreads(int t)
{
}

size_t OptiXManager::generateNID()
{
  return 0;
  // return ++_nid_counter;
}

Renderable* OptiXManager::getRenderable(size_t nid)
{
  return _map_renderables[nid];
}

void* OptiXManager::renderLoop(void* t)
{
}

void OptiXManager::internalRender()
{
}


void OptiXManager::render()
{
  printf("optix render\n");
  if (!initialized)
    return;
  if (next_scene->instances.size() == 0)
    return;
  for(vector<GRInstance>::iterator itr = next_scene->instances.begin(); itr != next_scene->instances.end(); itr++)
  {
    Manta::AffineTransform mt = itr->transform;
    Renderable* ren = itr->renderable;
    ORenderable* er = dynamic_cast<ORenderable*>(ren);
    if (er->isBuilt())
    {
        //JOAO: add instances to optix scene here
    }
  }
  next_scene->instances.resize(0);

  //JOAO: put render call here

  displayFrame();


      /* Primary RTAPI objects */
    RTcontext context;
    RTprogram ray_gen_program;
    RTbuffer  buffer;

    /* Parameters */
    RTvariable result_buffer;
    RTvariable draw_color;

    char path_to_ptx[512];
    char outfile[512];

    unsigned int width  = 512u;
    unsigned int height = 384u;
    int i;
    int use_glut = 1;

    outfile[0] = '\0';

    /* If "--file" is specified, don't do any GL stuff */
    // for( i = 1; i < argc; ++i ) {
    //   if( strcmp( argv[i], "--file" ) == 0 || strcmp( argv[i], "-f" ) == 0 )
    //     use_glut = 0;
    // }

    // /* Process command line args */
    // if( use_glut )
    //   RT_CHECK_ERROR_NO_CONTEXT( sutilInitGlut( &argc, argv ) );
    // for( i = 1; i < argc; ++i ) {
    //   if( strcmp( argv[i], "--help" ) == 0 || strcmp( argv[i], "-h" ) == 0 ) {
    //     printUsageAndExit( argv[0] );
    //   } else if( strcmp( argv[i], "--file" ) == 0 || strcmp( argv[i], "-f" ) == 0 ) {
    //     if( i < argc-1 ) {
    //       strcpy( outfile, argv[++i] );
    //     } else {
    //       printUsageAndExit( argv[0] );
    //     }
    //   } else if ( strncmp( argv[i], "--dim=", 6 ) == 0 ) {
    //     const char *dims_arg = &argv[i][6];
    //     if ( sutilParseImageDimensions( dims_arg, &width, &height ) != RT_SUCCESS ) {
    //       fprintf( stderr, "Invalid window dimensions: '%s'\n", dims_arg );
    //       printUsageAndExit( argv[0] );
    //     }
    //   } else {
    //     fprintf( stderr, "Unknown option '%s'\n", argv[i] );
    //     printUsageAndExit( argv[0] );
    //   }
    // }

    /* Create our objects and set state */
    RT_CHECK_ERROR( rtContextCreate( &context ) );
    RT_CHECK_ERROR( rtContextSetRayTypeCount( context, 1 ) );
    RT_CHECK_ERROR( rtContextSetEntryPointCount( context, 1 ) );

    RT_CHECK_ERROR( rtBufferCreate( context, RT_BUFFER_OUTPUT, &buffer ) );
    RT_CHECK_ERROR( rtBufferSetFormat( buffer, RT_FORMAT_FLOAT4 ) );
    RT_CHECK_ERROR( rtBufferSetSize2D( buffer, width, height ) );
    RT_CHECK_ERROR( rtContextDeclareVariable( context, "result_buffer", &result_buffer ) );
    RT_CHECK_ERROR( rtVariableSetObject( result_buffer, buffer ) );

    sprintf( path_to_ptx, "%s/%s", sutilSamplesPtxDir(), "sample1_generated_draw_color.cu.ptx" );
    printf("path_to_ptx: %s\n", path_to_ptx);
    RT_CHECK_ERROR( rtProgramCreateFromPTXFile( context, "/work/01336/carson/opt/optix/SDK-precompiled-samples/ptx/sample1_generated_draw_color.cu.ptx", "draw_solid_color", &ray_gen_program ) );
    RT_CHECK_ERROR( rtProgramDeclareVariable( ray_gen_program, "draw_color", &draw_color ) );
    RT_CHECK_ERROR( rtVariableSet3f( draw_color, 0.462f, 0.725f, 0.0f ) );
    RT_CHECK_ERROR( rtContextSetRayGenerationProgram( context, 0, ray_gen_program ) );

    /* Run */
    RT_CHECK_ERROR( rtContextValidate( context ) );
    RT_CHECK_ERROR( rtContextCompile( context ) );
    RT_CHECK_ERROR( rtContextLaunch2D( context, 0 /* entry point */, width, height ) );

    // /* Display image */
    // if( strlen( outfile ) == 0 ) {
    //   RT_CHECK_ERROR( sutilDisplayBufferInGlutWindow( argv[0], buffer ) );
    // } else {
    //   RT_CHECK_ERROR( sutilDisplayFilePPM( outfile, buffer ) );
    // }

      RTresult result;
  RTsize buffer_width, buffer_height;
  // int width, height;

  // Set the global RTcontext so we can destroy it at exit
  // if ( context != NULL ) {
  //   fprintf(stderr, "displayGlutWindow called, while another displayGlut is active.  Not supported.");
  //   // return RT_ERROR_UNKNOWN;
  //   return;
  // }
  // rtBufferGetContext( buffer, &context );
  
  // if ( !g_glut_initialized ) {
  //   fprintf(stderr, "displayGlutWindow called before initGlut.");
  //   return RT_ERROR_UNKNOWN;
  // }

  // result = checkBuffer(buffer);
  // if (result != RT_SUCCESS) {
  //   fprintf(stderr, "checkBuffer didn't pass\n");
  //   // return result;
  //   return;
  // }
  RTbuffer imageBuffer = buffer;

  result = rtBufferGetSize2D(buffer, &buffer_width, &buffer_height);
  if (result != RT_SUCCESS) {
    fprintf(stderr, "Error getting dimensions of buffer\n");
    // return result;
    return;
  }
  width  = static_cast<int>(buffer_width);
  height = static_cast<int>(buffer_height);

  //
  // display
  //
  GLvoid* imageData;
  // GLsizei width, height;
  // RTsize buffer_width, buffer_height;
  // RTsize buffer_height;
  RTformat buffer_format;

  result = rtBufferMap(imageBuffer, &imageData);
  if (result != RT_SUCCESS) {
    // Get error from context
    RTcontext context;
    const char* error;
    rtBufferGetContext(imageBuffer, &context);
    rtContextGetErrorString(context, result, &error);
    fprintf(stderr, "Error mapping image buffer: %s\n", error);
    exit(2);
  }
  if (0 == imageData) {
    fprintf(stderr, "data in buffer is null.\n");
    exit(2);
  }

  result = rtBufferGetSize2D(imageBuffer, &buffer_width, &buffer_height);
  if (result != RT_SUCCESS) {
    // Get error from context
    RTcontext context;
    const char* error;
    rtBufferGetContext(imageBuffer, &context);
    rtContextGetErrorString(context, result, &error);
    fprintf(stderr, "Error getting dimensions of buffer: %s\n", error);
    exit(2);
  }
  width  = static_cast<GLsizei>(buffer_width);
  height = static_cast<GLsizei>(buffer_height);

  result = rtBufferGetFormat(imageBuffer, &buffer_format);
  GLenum gl_data_type = GL_FALSE;
  GLenum gl_format = GL_FALSE;
  switch (buffer_format) {
    case RT_FORMAT_UNSIGNED_BYTE4:
      gl_data_type = GL_UNSIGNED_BYTE;
      gl_format    = GL_BGRA;
      break;

    case RT_FORMAT_FLOAT:
      gl_data_type = GL_FLOAT;
      gl_format    = GL_LUMINANCE;
      break;

    case RT_FORMAT_FLOAT3:
      gl_data_type = GL_FLOAT;
      gl_format    = GL_RGB;
      break;

    case RT_FORMAT_FLOAT4:
      gl_data_type = GL_FLOAT;
      gl_format    = GL_RGBA;
      break;

    default:
      fprintf(stderr, "Unrecognized buffer data type or format.\n");
      exit(2);
      break;
  }

  glDrawPixels(width, height, gl_format, gl_data_type, imageData);
  // glutSwapBuffers();

  // Now unmap the buffer
  result = rtBufferUnmap(imageBuffer);
  if (result != RT_SUCCESS) {
    // Get error from context
    RTcontext context;
    const char* error;
    rtBufferGetContext(imageBuffer, &context);
    rtContextGetErrorString(context, result, &error);
    fprintf(stderr, "Error unmapping image buffer: %s\n", error);
    exit(2);
  }


    /* Clean up */
    RT_CHECK_ERROR( rtBufferDestroy( buffer ) );
    RT_CHECK_ERROR( rtProgramDestroy( ray_gen_program ) );
    RT_CHECK_ERROR( rtContextDestroy( context ) );

}


void OptiXManager::displayFrame()
{
  char* data = NULL;

  //JOAO: map data to buffer

  if (!data)
    return;
  if (_format == "RGBA8")
    glDrawPixels(_width, _height, GL_RGBA, GL_UNSIGNED_BYTE, data);
  else
    return;

   if (params.write_to_file != "")
  {
    char* rgba_data = (char*)data;
    DEBUG("writing image\n");
    string filename = params.write_to_file;
      // if (params.write_to_file == "generated")
    {
      char cfilename[256];
#if USE_MPI
      sprintf(cfilename, "render_%04d_%dx%d_%d.rgb", _realFrameNumber, _width, _height, _rank);
#else
      sprintf(cfilename, "render_%04d_%dx%d.rgb", _realFrameNumber, _width, _height);
#endif
      filename = string(cfilename);
    }

    printf("writing filename: %s\n", filename.c_str());

      //unsigned char* test = new unsigned char[xres*yres*3];
      //glReadPixels(0,0,xres,yres,GL_RGB, GL_UNSIGNED_BYTE, test);
    FILE* pFile = fopen(filename.c_str(), "w");
    assert(pFile);
    if (_format == "RGBA8")
    {
      fwrite((void*)&rgba_data[0], 1, _width*_height*4, pFile);
      fclose(pFile);
      stringstream s("");
        //TODO: this fudge factor on teh sizes makes no sense... I'm assuming it's because they have row padding in the data but it doesn't show up in drawpixels... perplexing.  It can also crash just a hack for now
      s  << "convert -flip -size " << _width << "x" << _height << " -depth 8 rgba:" << filename << " " << filename << ".png && rm " << filename ;
        /*printf("calling system call \"%s\"\n", s.str().c_str());*/
      system(s.str().c_str());
        //delete []test;

    }
    else
    {
      fwrite(data, 1, _width*_height*3, pFile);
      fclose(pFile);
      stringstream s("");
        //TODO: this fudge factor on teh sizes makes no sense... I'm assuming it's because they have row padding in the data but it doesn't show up in drawpixels... perplexing.  It can also crash just a hack for now
      s << "convert -flip -size " << _width << "x" << _height << " -depth 8 rgb:" << filename << " " << filename << ".png && rm " << filename;
      system(s.str().c_str());
    }
      //delete []test;
    _realFrameNumber++;
  }
}

void OptiXManager::syncInstances()
{}

void OptiXManager::updateCamera()
{
  //JOAO: put camera update here
}

void OptiXManager::updateBackground()
{
}

void OptiXManager::addInstance(Renderable* ren)
{
  if (!ren->isBuilt())
  {
    std::cerr << "addInstance: renderable not build by rendermanager\n";
    return;
  }
  next_scene->instances.push_back(GRInstance(ren, current_transform));
}

void OptiXManager::addRenderable(Renderable* ren)
{
  ORenderable* oren = dynamic_cast<ORenderable*>(ren);
  if (!oren)
  {
    printf("error: OptiXManager::addRenderable wrong renderable type\n");
    return;
  }
  // updateMaterial();
      // msgModel = new miniSG::Model;
      // msgModel->material.push_back(new miniSG::Material);
  // OSPMaterial ospMat = ospNewMaterial(renderer,"OBJMaterial");
  // float Kd[] = {1.f,1.f,1.f};
  // float Ks[] = {1,1,1};

  Manta::Mesh* mesh = oren->_data->mesh;
  assert(mesh);
  size_t numNormals = mesh->vertexNormals.size();
  size_t numTexCoords = mesh->texCoords.size();
  size_t numPositions = mesh->vertices.size();
  printf("addrenderable called mesh indices/3 vertices normals texcoords: %d %d %d %d \n", mesh->vertex_indices.size()/3, mesh->vertices.size(), mesh->vertexNormals.size(),
    mesh->texCoords.size());
  size_t numTriangles = mesh->vertex_indices.size()/3;
  // assert(mesh->vertices.size() == numTriangles*3);
  oren->setBuilt(true);

  //JOAO: put optix call here

  std::string            m_pathname;
  std::string            m_filename;
  optix::Context         m_context;
  optix::GeometryGroup   m_geometrygroup;
  optix::Buffer          m_vbuffer;
  optix::Buffer          m_nbuffer;
  optix::Buffer          m_tbuffer;
  optix::Material        m_material;
  optix::Program         m_intersect_program;
  optix::Program         m_bbox_program;
  optix::Buffer          m_light_buffer;
  bool                   m_have_default_material;
  bool                   m_force_load_material_params;
  const char*            m_ASBuilder;
  const char*            m_ASTraverser;
  const char*            m_ASRefine;
  bool                   m_large_geom;
  optix::Aabb            m_aabb;
  // std::vector<MatParams> m_material_params;


  //  GLMmodel* model = new GLModel();
  //  // model->pathname      = strdup(filename);
  //  model->pathname = NULL;
  //  model->mtllibname    = NULL;
  // model->numvertices   = 0;
  // model->vertices      = NULL;
  // model->vertexColors  = NULL;
  // model->numnormals    = 0;
  // model->normals       = NULL;
  // model->numtexcoords  = 0;
  // model->texcoords     = NULL;
  // model->numfacetnorms = 0;
  // model->facetnorms    = NULL;
  // model->numtriangles  = 0;
  // model->triangles     = NULL;
  // model->nummaterials  = 0;
  // model->materials     = NULL;
  // model->numgroups     = 0;
  // model->groups        = NULL;
  // model->position[0]   = 0.0;
  // model->position[1]   = 0.0;
  // model->position[2]   = 0.0;
  // model->usePerVertexColors = 0;

  //   model->vertices = (float*)malloc(sizeof(float) *
  //     3 * (numPositions + 1));
  // // model->vertexColors = (unsigned char*)malloc(sizeof(unsigned char) *
  //     // 3 * ( + 1));
  // model->triangles = (GLMtriangle*)malloc(sizeof(GLMtriangle) *
  //     model->numtriangles);
  // if (model->numnormals) {
  //   model->normals = (float*)malloc(sizeof(float) *
  //       3 * (model->numnormals + 1));
  // }
  // if (model->numtexcoords) {
  //   model->texcoords = (float*)malloc(sizeof(float) *
  //       2 * (model->numtexcoords + 1));
  // }

//
//material
  //
    std::string path = std::string("/work/01336/carson/opt/optix/SDK-precompiled-samples/ptx/") + "/cuda_compile_ptx_generated_obj_material.cu.ptx";

  Program closest_hit = context->createProgramFromPTXFile( path, "closest_hit_radiance" );
  Program any_hit     = context->createProgramFromPTXFile( path, "any_hit_shadow" );
  m_material           = context->createMaterial();
  m_material->setClosestHitProgram( 0u, closest_hit );
  m_material->setAnyHitProgram( 1u, any_hit );


//
  // vertex data
  //
  //   unsigned int num_vertices  = model->numvertices;
  // unsigned int num_texcoords = model->numtexcoords;
  // unsigned int num_normals   = model->numnormals;
  size_t num_vertices = numPositions;
  num_texcoords = 0;
  num_normals = numNormals;

  // Create vertex buffer
  m_vbuffer = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, numPositions );
  float3* vbuffer_data = static_cast<float3*>( m_vbuffer->map() );

  // Create normal buffer
  m_nbuffer = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, num_normals );
  float3* nbuffer_data = static_cast<float3*>( m_nbuffer->map() );

  // Create texcoord buffer
  m_tbuffer = context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_FLOAT2, num_texcoords );
  float2* tbuffer_data = static_cast<float2*>( m_tbuffer->map() );

  // Transform and copy vertices.  
  for ( unsigned int i = 0; i < num_vertices; ++i )
  {
    const float3 v3 = *((float3*)&mesh->vertices[(i+1)*3]);
    float4 v4 = make_float4( v3, 1.0f );
    vbuffer_data[i] = make_float3( transform*v4 );
  }

  // Transform and copy normals.
  const optix::Matrix4x4 norm_transform = transform.inverse().transpose();
  for( unsigned int i = 0; i < num_normals; ++i )
  {
    const float3 v3 = *((float3*)&mesh->normal_vertices[(i+1)*3]);
    float4 v4 = make_float4( v3, 0.0f );
    nbuffer_data[i] = make_float3( norm_transform*v4 );
  }

  // Copy texture coordinates.
  // memcpy( static_cast<void*>( tbuffer_data ),
  //         static_cast<void*>( &(model->texcoords[2]) ),
  //         sizeof( float )*num_texcoords*2 );   

  // Calculate bbox of model
  for( unsigned int i = 0; i < num_vertices; ++i )
    m_aabb.include( vbuffer_data[i] );

  // Unmap buffers.
  m_vbuffer->unmap();
  m_nbuffer->unmap();
  m_tbuffer->unmap();


  //
  // set materials
  //
    // GLMmaterial& mat = model->materials[i];
    // MatParams& params = m_material_params[i];

    params.emissive     = make_float3( 0,0,0 );
    params.reflectivity = make_float3( 1,1,1 );
    params.phong_exp    = 10; 
    params.illum        = 2;//( (mat.shader > 3) ? 2 : mat.shader ); // use 2 as default if out-of-range

    float3 Kd = make_float3( .2,
                             .5,
                             .8 );
    float3 Ka = make_float3( .4,
                             .4,
                             .4 );
    float3 Ks = make_float3( 1,
                             1,
                             1 );

    // load textures relatively to OBJ main file
    // std::string ambient_map  = strlen(mat.ambient_map)  ? m_pathname + mat.ambient_map  : "";
    // std::string diffuse_map  = strlen(mat.diffuse_map)  ? m_pathname + mat.diffuse_map  : "";
    // std::string specular_map = strlen(mat.specular_map) ? m_pathname + mat.specular_map : "";

    // params.ambient_map = loadTexture( m_context, ambient_map, Ka );
    // params.diffuse_map = loadTexture( m_context, diffuse_map, Kd );
    // params.specular_map = loadTexture( m_context, specular_map, Ks );

    //
    // instances
    //
    // createGeometryInstances(model);

    // createLightBuffer( model );

    // glmDelete( model );
}

#if 0
void OptiXManager::createLightBuffer( GLMmodel* model )
{
  // create a buffer for the next-event estimation
  m_light_buffer = m_context->createBuffer( RT_BUFFER_INPUT );
  m_light_buffer->setFormat( RT_FORMAT_USER );
  m_light_buffer->setElementSize( sizeof( TriangleLight ) );

  // light sources
  std::vector<TriangleLight> lights;

  unsigned int num_light = 0u;
  unsigned int group_count = 0u;

  if (model->nummaterials > 0)
  {
    for ( GLMgroup* obj_group = model->groups; obj_group != 0; obj_group = obj_group->next, group_count++ ) 
    {
      unsigned int num_triangles = obj_group->numtriangles;
      if ( num_triangles == 0 ) continue; 
      GLMmaterial& mat = model->materials[obj_group->material];

      if ( (mat.emissive[0] + mat.emissive[1] + mat.emissive[2]) > 0.0f ) 
      {
        // extract necessary data
        for ( unsigned int i = 0; i < obj_group->numtriangles; ++i ) 
        {
          // indices for vertex data
          unsigned int tindex = obj_group->triangles[i];
          int3 vindices;
          vindices.x = model->triangles[ tindex ].vindices[0]; 
          vindices.y = model->triangles[ tindex ].vindices[1]; 
          vindices.z = model->triangles[ tindex ].vindices[2]; 

          TriangleLight light;
          light.v1 = *((float3*)&model->vertices[vindices.x * 3]);
          light.v2 = *((float3*)&model->vertices[vindices.y * 3]);
          light.v3 = *((float3*)&model->vertices[vindices.z * 3]);

          // normal vector
          light.normal = normalize( cross( light.v2 - light.v3, light.v1 - light.v3 ) );

          light.emission = make_float3( mat.emissive[0], mat.emissive[1], mat.emissive[2] );

          lights.push_back(light);
          
          num_light++;
        }
      }
    }
  }

  // write to the buffer
  m_light_buffer->setSize( 0 );
  if (num_light != 0)
  {
    m_light_buffer->setSize( num_light );
    memcpy( m_light_buffer->map(), &lights[0], num_light * sizeof( TriangleLight ) );
    m_light_buffer->unmap();
  }
}
#endif

#if 0
void OptiXManager::createGeometryInstances( GLMmodel* model )
{
  // Load triangle_mesh programs
  if( !m_intersect_program.get() ) {
    std::string path = std::string("/work/01336/carson/opt/optix/SDK-precompiled-samples/ptx/") + "/cuda_compile_ptx_generated_triangle_mesh.cu.ptx";
    m_intersect_program = m_context->createProgramFromPTXFile( path, "mesh_intersect" );
  }

  if( !m_bbox_program.get() ) {
    std::string path = std::string("/work/01336/carson/opt/optix/SDK-precompiled-samples/ptx/" + "/cuda_compile_ptx_generated_triangle_mesh.cu.ptx";
    m_bbox_program = m_context->createProgramFromPTXFile( path, "mesh_bounds" );
  }

  std::vector<GeometryInstance> instances;

  // Loop over all groups -- grab the triangles and material props from each group
  unsigned int triangle_count = 0u;
  unsigned int group_count = 0u;
  for ( GLMgroup* obj_group = model->groups;
        obj_group != 0;
        obj_group = obj_group->next, group_count++ ) {

    unsigned int num_triangles = obj_group->numtriangles;
    if ( num_triangles == 0 ) continue; 

    // Create vertex index buffers
    Buffer vindex_buffer = m_context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_INT3, num_triangles );
    int3* vindex_buffer_data = static_cast<int3*>( vindex_buffer->map() );

    Buffer tindex_buffer = m_context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_INT3, num_triangles );
    int3* tindex_buffer_data = static_cast<int3*>( tindex_buffer->map() );

    Buffer nindex_buffer = m_context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_INT3, num_triangles );
    int3* nindex_buffer_data = static_cast<int3*>( nindex_buffer->map() );

    Buffer mbuffer = m_context->createBuffer( RT_BUFFER_INPUT, RT_FORMAT_UNSIGNED_INT, num_triangles );
    unsigned int* mbuffer_data = static_cast<unsigned int*>( mbuffer->map() );

    optix::Geometry mesh;

    for ( unsigned int i = 0; i < numTriangles; ++i, ++numTriangles ) {

      // unsigned int tindex = mesh->indices[i];
      int3 vindices;
      // vindices.x = mesh->indices[ tindex ].vindices[0] - 1; 
      // vindices.y = model->triangles[ tindex ].vindices[1] - 1; 
      // vindices.z = model->triangles[ tindex ].vindices[2] - 1; 
      vindices.x = mesh->vertex_indices[i*3+0];
      vindices.y = mesh->vertex_indices[i*3+1];
      vindices.z = mesh->vertex_indices[i*3+2];
      assert( vindices.x <= static_cast<int>(numTriangles*3) );
      assert( vindices.y <= static_cast<int>(numTriangles*3) );
      assert( vindices.z <= static_cast<int>(numTriangles*3) );
      
      vindex_buffer_data[ i ] = vindices;

      int3 nindices;
      // nindices.x = model->triangles[ tindex ].nindices[0] - 1; 
      // nindices.y = model->triangles[ tindex ].nindices[1] - 1; 
      // nindices.z = model->triangles[ tindex ].nindices[2] - 1; 
      nindices.x = mesh->normal_indices[i*3+0];
      nindices.y = mesh->normal_indices[i*3+1];
      // nindices.z = mesh->normal_indices[i*3+2];
      // assert( nindices.x <= static_cast<int>(model->numnormals) );
      // assert( nindices.y <= static_cast<int>(model->numnormals) );
      // assert( nindices.z <= static_cast<int>(model->numnormals) );

      // int3 tindices;
      // tindices.x = model->triangles[ tindex ].tindices[0] - 1; 
      // tindices.y = model->triangles[ tindex ].tindices[1] - 1; 
      // tindices.z = model->triangles[ tindex ].tindices[2] - 1; 
      // assert( tindices.x <= static_cast<int>(model->numtexcoords) );
      // assert( tindices.y <= static_cast<int>(model->numtexcoords) );
      // assert( tindices.z <= static_cast<int>(model->numtexcoords) );

      nindex_buffer_data[ i ] = nindices;
      tindex_buffer_data[ i ] = tindices;
      mbuffer_data[ i ] = 0; // See above TODO

    }
    vindex_buffer->unmap();
    tindex_buffer->unmap();
    nindex_buffer->unmap();
    mbuffer->unmap();

    std::vector<int> tri_reindex;

    // if (m_large_geom) {
    //   if( m_ASBuilder == std::string("Sbvh") || m_ASBuilder == std::string("KdTree")) {
    //     m_ASBuilder = "MedianBvh";
    //     m_ASTraverser = "Bvh";
    //   }

    //   float* vertex_buffer_data = static_cast<float*>( m_vbuffer->map() );
    //   RTsize nverts;
    //   m_vbuffer->getSize(nverts);

    //   tri_reindex.resize(num_triangles);
    //   RTgeometry geometry;

    //   unsigned int usePTX32InHost64 = 0;
    //   rtuCreateClusteredMeshExt( context->get(), usePTX32InHost64, &geometry, 
    //                             (unsigned int)nverts, vertex_buffer_data, 
    //                              num_triangles, (const unsigned int*)vindex_buffer_data,
    //                              mbuffer_data,
    //                              m_nbuffer->get(), 
    //                             (const unsigned int*)nindex_buffer_data,
    //                             m_tbuffer->get(), 
    //                             (const unsigned int*)tindex_buffer_data);
    //   mesh = optix::Geometry::take(geometry);

    //   m_vbuffer->unmap();
    //   rtBufferDestroy( vindex_buffer->get() );
    // } else 
    {
      // Create the mesh object
      mesh = context->createGeometry();
      mesh->setPrimitiveCount( num_triangles );
      mesh->setIntersectionProgram( m_intersect_program);
      mesh->setBoundingBoxProgram( m_bbox_program );
      mesh[ "vertex_buffer" ]->setBuffer( m_vbuffer );
      mesh[ "vindex_buffer" ]->setBuffer( vindex_buffer );
      mesh[ "normal_buffer" ]->setBuffer( m_nbuffer );
      mesh[ "texcoord_buffer" ]->setBuffer( m_tbuffer );
      mesh[ "tindex_buffer" ]->setBuffer( tindex_buffer );
      mesh[ "nindex_buffer" ]->setBuffer( nindex_buffer );
      mesh[ "material_buffer" ]->setBuffer( mbuffer );
    }

    // Create the geom instance to hold mesh and material params
    GeometryInstance instance = context->createGeometryInstance( mesh, &m_material, &m_material+1 );
    // loadMaterialParams( instance, obj_group->material );
    instances.push_back( instance );
  }

  // assert( triangle_count == model->numtriangles );
  
  // Set up group 
  const unsigned current_child_count = m_geometrygroup->getChildCount();
  m_geometrygroup->setChildCount( current_child_count + static_cast<unsigned int>(instances.size()) );
  optix::Acceleration acceleration = m_context->createAcceleration(m_ASBuilder, m_ASTraverser);
  acceleration->setProperty( "refine", m_ASRefine );
  if (m_large_geom) {
    acceleration->setProperty( "leaf_size", "1" );
  } else {
    if ( m_ASBuilder   == std::string("Sbvh") ||
         m_ASBuilder   == std::string("Trbvh") ||
         m_ASBuilder   == std::string("TriangleKdTree") ||
         m_ASTraverser == std::string( "KdTree" )) {
      acceleration->setProperty( "vertex_buffer_name", "vertex_buffer" );
      acceleration->setProperty( "index_buffer_name", "vindex_buffer" );
    }
  }
  m_geometrygroup->setAcceleration( acceleration );
  acceleration->markDirty();

  for ( unsigned int i = 0; i < instances.size(); ++i )
    m_geometrygroup->setChild( current_child_count + i, instances[i] );

  if (m_large_geom) {
    rtBufferDestroy( m_vbuffer->get() );
  }
}
#endif

void OptiXManager::deleteRenderable(Renderable* ren)
{
  //TODO: DELETE RENDERABLES
  ORenderable* r = dynamic_cast<ORenderable*>(ren);
  // printf("deleting renderable of size: %d\n", er->_data->mesh->vertex_indices.size()/3);
  /*if (er->isBuilt())*/
  /*g_device->rtClear(er->_data->d_mesh);*/
  r->setBuilt(false);
  /*er->_data->mesh->vertexNormals.resize(0);*/
  // delete er->_data->mesh;  //embree handles clearing the data... not sure how to get it to not do that with rtclear yet
}



void OptiXManager::addTexture(int handle, int target, int level, int internalFormat, int width, int height, int border, int format, int type, void* data)
{
}

void OptiXManager::deleteTexture(int handle)
{

}

GeometryGenerator* OptiXManager::getGeometryGenerator(int type)
{
  switch(type)
  {
    case GL_TRIANGLES:
    {
      return _gTriangle;
    }
    case GL_TRIANGLE_STRIP:
    {
      return _gTriangleStrip;
    }
    case GL_QUADS:
    {
      return _gQuads;
    }
    case GL_QUAD_STRIP:
    {
      return _gQuadStrip;
    }
    //case GL_LINES:
    //{
        ////			gen = rm->GLines;
        ////break;
    //}
    //case GL_LINE_STRIP:
    //{
        ////			gen = rm->GLineStrip;
        ////break;
    //}
    //case GL_POLYGON:
    //{
        ////this is temporary for visit, need to support other than quads
        ////break;
    //}
    default:
    {
      return _gVoid;
    }
  }
}

RenderManager* createOptiXManager(){ return OptiXManager::singleton(); }
