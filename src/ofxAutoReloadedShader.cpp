
#include "ofxAutoReloadedShader.h"

#ifdef __APPLE__
#else
using namespace std::chrono;
#endif


ofxAutoReloadedShader::ofxAutoReloadedShader(){
	bWatchingFiles = false;
}

ofxAutoReloadedShader::~ofxAutoReloadedShader(){
	disableWatchFiles();
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
//
bool ofxAutoReloadedShader::load(const std::filesystem::path& shaderName )
{	return load( shaderName.string() + ".vert", shaderName.string() + ".frag", shaderName.string() + ".geom");
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
//
bool ofxAutoReloadedShader::load(const std::filesystem::path& vertName, const std::filesystem::path& fragName, const std::filesystem::path& geomName)
{
	unload();
	
    ofShader::setGeometryOutputCount(geometryOutputCount);
    ofShader::setGeometryInputType(geometryInputType);
    ofShader::setGeometryOutputType(geometryOutputType);

    
	// hackety hack, clear errors or shader will fail to compile
	GLuint err = glGetError();
	
	lastTimeCheckMillis = ofGetElapsedTimeMillis();
	setMillisBetweenFileCheck( 2 * 1000 );
	enableWatchFiles();
	
	loadShaderNextFrame = false;
	
	vertexShaderFilename = ofFilePath::getAbsolutePath(vertName, true);
	fragmentShaderFilename = ofFilePath::getAbsolutePath(fragName, true);
	geometryShaderFilename = ofFilePath::getAbsolutePath(geomName, true);
	
	vertexShaderFile.clear();
	fragmentShaderFile.clear();
	geometryShaderFile.clear();
	
	vertexShaderFile   = ofFile(vertName);
	fragmentShaderFile = ofFile(fragName);
    geometryShaderFile = ofFile(geomName);
	
	ofBuffer vertexShaderBuffer = ofBufferFromFile(vertName);
	ofBuffer fragmentShaderBuffer = ofBufferFromFile(fragName);
	ofBuffer geometryShaderBuffer = ofBufferFromFile(geomName);

	
	fileChangedTimes.clear();
	fileChangedTimes.push_back( getLastModified( vertexShaderFile ) );
	fileChangedTimes.push_back( getLastModified( fragmentShaderFile ) );
	fileChangedTimes.push_back( getLastModified( geometryShaderFile ) );
	
	if( vertexShaderBuffer.size() > 0 )
	{
		string sourceDirectoryPath = ofFilePath::getEnclosingDirectory(vertexShaderFilename, false);
		setupShaderFromSource(GL_VERTEX_SHADER, vertexShaderBuffer.getText(), sourceDirectoryPath);
	}

	if( fragmentShaderBuffer.size() > 0 )
	{
		string sourceDirectoryPath = ofFilePath::getEnclosingDirectory(fragmentShaderFilename, false);
		setupShaderFromSource(GL_FRAGMENT_SHADER, fragmentShaderBuffer.getText(), sourceDirectoryPath);
	}

	#ifndef TARGET_OPENGLES
	if( geometryShaderBuffer.size() > 0 )
	{
		string sourceDirectoryPath = ofFilePath::getEnclosingDirectory(geometryShaderFilename, false);
		setupShaderFromSource(GL_GEOMETRY_SHADER_EXT, geometryShaderBuffer.getText(), sourceDirectoryPath);
	}
	#endif

	bindDefaults();
	
	return linkProgram();
}

bool ofxAutoReloadedShader::loadCompute(const of::filesystem::path& shaderName)
{
	unload();

	// hackety hack, clear errors or shader will fail to compile
	GLuint err = glGetError();

	lastTimeCheckMillis = ofGetElapsedTimeMillis();
	setMillisBetweenFileCheck(2 * 1000);
	enableWatchFiles();

	loadShaderNextFrame = false;

	computeShaderFilename = ofFilePath::getAbsolutePath(shaderName, true);
	computeShaderFile.clear();
	computeShaderFile = ofFile(shaderName);

	ofBuffer computeShaderBuffer = ofBufferFromFile(shaderName);

	fileChangedTimes.clear();
	fileChangedTimes.push_back(getLastModified(computeShaderFile));
	if (computeShaderBuffer.size() > 0)
	{
		string sourceDirectoryPath = ofFilePath::getEnclosingDirectory(computeShaderFilename, false);
		setupShaderFromSource(GL_COMPUTE_SHADER, computeShaderBuffer.getText(), sourceDirectoryPath);
	}

	bindDefaults();
	this->isCompute = true;

	return linkProgram();

}



// ---------------------------------------------------------------------------------------------------------------------------------------------------
//
void ofxAutoReloadedShader::_update(ofEventArgs &e)
{
	cout << computeShaderFilename << endl;
	if( loadShaderNextFrame )
	{
		reloadShaders();
		loadShaderNextFrame = false;
	}
	
	int currTime = ofGetElapsedTimeMillis();
	
	if( ((currTime - lastTimeCheckMillis) > millisBetweenFileCheck) &&
	   !loadShaderNextFrame )
	{
		if( filesChanged() )
		{
			loadShaderNextFrame = true;
		}
		
		lastTimeCheckMillis = currTime;
	}
}


// ---------------------------------------------------------------------------------------------------------------------------------------------------
//
bool ofxAutoReloadedShader::reloadShaders()
{
	if (isCompute) {
		return loadCompute(computeShaderFilename);
	}
	else {
		return load(vertexShaderFilename, fragmentShaderFilename, geometryShaderFilename);
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
//
void ofxAutoReloadedShader::enableWatchFiles()
{
	if(!bWatchingFiles){
		ofAddListener(ofEvents().update, this, &ofxAutoReloadedShader::_update );
		bWatchingFiles = true;
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
//
void ofxAutoReloadedShader::disableWatchFiles()
{
	if(bWatchingFiles){
		ofRemoveListener(ofEvents().update, this, &ofxAutoReloadedShader::_update );
		bWatchingFiles = false;
	}
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
//
bool ofxAutoReloadedShader::filesChanged()
{
	bool fileChanged = false;
	
	if( vertexShaderFile.exists() )
	{
		std::time_t vertexShaderFileLastChangeTime = getLastModified( vertexShaderFile );
		if( vertexShaderFileLastChangeTime != fileChangedTimes.at(0) )
		{
			fileChangedTimes.at(0) = vertexShaderFileLastChangeTime;
			fileChanged = true;
		}
	}
	
	if( fragmentShaderFile.exists() )
	{
		std::time_t fragmentShaderFileLastChangeTime = getLastModified( fragmentShaderFile );
		if( fragmentShaderFileLastChangeTime != fileChangedTimes.at(1) )
		{
			fileChangedTimes.at(1) = fragmentShaderFileLastChangeTime;
			fileChanged = true;
		}
	}
		
	if( geometryShaderFile.exists() )
	{
		std::time_t geometryShaderFileLastChangeTime = getLastModified( geometryShaderFile );
		if( geometryShaderFileLastChangeTime != fileChangedTimes.at(2) )
		{
			fileChangedTimes.at(2) = geometryShaderFileLastChangeTime;
			fileChanged = true;
		}
	}

	if (computeShaderFile.exists())
	{
		std::time_t computeShaderFileLastChangeTime = getLastModified(computeShaderFile);
		if (computeShaderFileLastChangeTime != fileChangedTimes.at(0))
		{
			fileChangedTimes.at(0) = computeShaderFileLastChangeTime;
			fileChanged = true;
		}

	}

	return fileChanged;
}

// ---------------------------------------------------------------------------------------------------------------------------------------------------
//
std::time_t ofxAutoReloadedShader::getLastModified( ofFile& _file )
{
	if( _file.exists() )
	{
#ifdef __APPLE__
    return std::filesystem::last_write_time(_file.path());
#else
	std:filesystem::file_time_type ftt = std::filesystem::last_write_time(_file.path());
    return to_time_t(ftt);
#endif
        
	}
	else
	{
		return 0;
	}
}

#ifdef __APPLE__
#else
template <typename TP>
std::time_t ofxAutoReloadedShader::to_time_t(TP tp)
{
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
        + system_clock::now());
    return system_clock::to_time_t(sctp);
}
#endif


// ---------------------------------------------------------------------------------------------------------------------------------------------------
//
void ofxAutoReloadedShader::setMillisBetweenFileCheck( int _millis )
{
	millisBetweenFileCheck = _millis;
}

//--------------------------------------------------------------
void ofxAutoReloadedShader::setGeometryInputType(GLenum type) {
    ofShader::setGeometryInputType(type);
    geometryInputType = type;
}

//--------------------------------------------------------------
void ofxAutoReloadedShader::setGeometryOutputType(GLenum type) {
    ofShader::setGeometryOutputType(type);
    geometryOutputType = type;
}

//--------------------------------------------------------------
void ofxAutoReloadedShader::setGeometryOutputCount(int count) {
    ofShader::setGeometryOutputCount(count);
    geometryOutputCount = count;
}
