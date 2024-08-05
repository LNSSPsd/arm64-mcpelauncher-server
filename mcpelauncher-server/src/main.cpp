#include <log.h>
#include <dlfcn.h>
#include <argparser.h>
#include <mcpelauncher/minecraft_utils.h>
#include <mcpelauncher/minecraft_version.h>
#include <mcpelauncher/crash_handler.h>
#include <mcpelauncher/path_helper.h>
#include <mcpelauncher/mod_loader.h>
#include <mcpelauncher/patch_utils.h>
#include <libc_shim.h>
#include <mcpelauncher/linker.h>
#include <minecraft/imported/android_symbols.h>
#include "stubs.h"
#include <FileUtil.h>
#include <properties/property.h>
#include <fstream>
#include <signal.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/mman.h>
#include <bitset>
#include <memory.h>
#include <memory>
#include <random>
#include "main.h"
#include "console_reader.h"


void printVersionInfo();

int main(int argc, char* argv[]) {
	CrashHandler::registerCrashHandler();
	MinecraftUtils::workaroundLocaleBug();

	argparser::arg_parser p;
	argparser::arg<bool> printVersion(p, "--version", "-v", "Prints version info");
	argparser::arg<std::string> propertiesFileArg(p, "--properties", "-p", "server.properties file");

	std::string propertiesFilePath="server.properties";

	if(!p.parse(argc, (const char**)argv))
		return 1;
	if(printVersion) {
		printVersionInfo();
		return 0;
	}

	if(!propertiesFileArg.get().empty())
		propertiesFilePath=propertiesFileArg;

	Log::info("Launcher", "Version: arm64-server");
	Log::info("Launcher", "Loader server.properties");
	std::ifstream propertiesFile(propertiesFilePath);
	if(!propertiesFile) {
		Log::error("Launcher", "Unable to read properties file: %s", propertiesFilePath.c_str());
		return 2;
	}
	properties::property_list prop;
	prop.load(propertiesFile);

	Log::trace("Launcher", "Loading android libraries");
	linker::init();
	Log::trace("Launcher", "linker loaded");

	properties::property<std::string> game_dir(prop, "game-directory", "game");
	properties::property<std::string> data_dir(prop, "data-directory", "data");
	PathHelper::setGameDir(game_dir);
	Log::info("Launcher", "Game directory: %s", game_dir.get().c_str());
	Log::info("Launcher", "Data directory: %s", data_dir.get().c_str());

	// Fix saving to internal storage without write access to /data/*
	// TODO research how this path is constructed
	auto pid = getpid();
	shim::rewrite_filesystem_access = {
		{"worlds", data_dir.get()+"/worlds/"},
		{"permissions.json", data_dir.get()+"/permissions.json"},
		{"data", data_dir.get()+"/"},
		{"treatments", data_dir.get()+"/treatments/"},
		{"minecraftpe", data_dir.get()+"/minecraftpe/"},
		{"premium_cache", data_dir.get()+"/premium_cache/"},
		{".", PathHelper::getGameDir()+"assets/"}
	};
	for(auto&& redir : shim::rewrite_filesystem_access) {
		Log::trace("REDIRECT", "%s to %s", redir.first.data(), redir.second.data());
	}
	auto libC = MinecraftUtils::getLibCSymbols();
	linker::load_library("libc.so", libC);
	MinecraftUtils::loadLibM();
	MinecraftUtils::setupHybris();
	try {
		PathHelper::findGameFile(std::string("lib/") + MinecraftUtils::getLibraryAbi() + "/libminecraftpe.so");
	} catch(std::exception& e) {
		Log::error("Launcher", "Could not find the game, use the game-directory property to fix this error. Original Error: %s", e.what());
		return 1;
	}
	linker::update_LD_LIBRARY_PATH(PathHelper::findGameFile(std::string("lib/") + MinecraftUtils::getLibraryAbi()).data());
	/*if(!disableFmod) {
		try {
			MinecraftUtils::loadFMod();
		} catch(std::exception& e) {
			Log::warn("FMOD", "Failed to load host libfmod: '%s', use experimental pulseaudio backend if available", e.what());
		}
	}*/

	std::unordered_map<std::string, void*> android_syms;
	Stubs::initHybrisHooks(android_syms);
	for(auto s = android_symbols; *s; s++)  // stub missing symbols
		android_syms.insert({*s, (void*)+[]() { Log::warn("Main", "Android stub called"); }});
	linker::load_library("libandroid.so", android_syms);
	ModLoader modLoader;
	modLoader.loadModsFromDirectory(PathHelper::getPrimaryDataDirectory() + "mods/", true);
	linker::load_library("libGLESv2.so", {});
	linker::load_library("libEGL.so", {});

	Log::trace("Launcher", "Loading Minecraft library");
	static void* handle = MinecraftUtils::loadMinecraftLib();
	if(!handle) {
		Log::error("Launcher", "Failed to load Minecraft library");
		return 51;
	}
	Log::info("Launcher", "Loaded Minecraft library");
	Log::debug("Launcher", "Minecraft is at offset %p", (void*)MinecraftUtils::getLibraryBase(handle));
	base = MinecraftUtils::getLibraryBase(handle);
	if(*(uint64_t*)(base+0x38B4309)!=0x1b011e7c0100527aL) {
		Log::error("Launcher", "Incompatible Minecraft version, only v1.21.2.02 is supported.");
		return 52;
	}

	modLoader.loadModsFromDirectory(PathHelper::getPrimaryDataDirectory() + "mods/");

	Log::info("Launcher", "Game version: v1.21.2.02");

	//Log::info("Launcher", "SERVER!");
	Log::debug("Launcher", "Creating ContentLog");
	ContentLog *contentLog=new ContentLog;
	Log::debug("Launcher", "Creating AppConfigs");
	std::unique_ptr<AppConfigs> appConfigs=AppConfigsFactory::createAppConfigs();
	((ServiceReference(*)(ContentLog*))(base+0x61CFC5C))(contentLog);
	((ServiceReference(*)(AppConfigs*))(base+0x6218250))(appConfigs.get());
	Log::info("Launcher", "Constructing AppPlatform");
	AppPlatform *AppPlatform_obj=new AppPlatform(true);
	Log::info("Launcher", "Patching AppPlatform vtable");
	void **myvtable=(void**)malloc(2750);
	memcpy((void*)myvtable,*(void**)AppPlatform_obj, 2750);
	myvtable[15]=(void*)(uint64_t(*)(void))[]()->uint64_t {
		// getBuildPlatform
		return 15;
	};
	myvtable[19]=(void*)(uint64_t(*)(void))[]()->uint64_t {
		// getTotalPhysicalMemory
		return 17179869184;
	};
	myvtable[28]=(void*)(std::string(*)(void))[]()->std::string {
		//Log::info("AppPlatform","getPackagePath");
		return "";
	};
	myvtable[164]=(void*)(void(*)(void))[]() {
		Log::error("AppPlatform", "Fatal: queueForMainThread_DEPRECATED IS NOT IMPLEMENTED!!");
		_Exit(1);
	};
	myvtable[165]=(void*)(void(*)(void))[]() {
		Log::error("AppPlatform", "Fatal: getMainThreadQueue IS NOT IMPLEMENTED!!");
		_Exit(1);
	};
	myvtable[206]=(void*)(uint64_t(*)(void))[]()->uint64_t {
		// getPlatformType
		return 0;
	};
	myvtable[209]=(void*)(std::string(*)(void))[]()->std::string {
		return "Linux";
	};
	myvtable[210]=(void*)(std::string(*)(void))[]()->std::string {
		return "Dedicated";
	};
	myvtable[221]=(void*)(std::string(*)(void))[]()->std::string {
		return "com.mojang.minecraftpe_server";
	};
	myvtable[222]=(void*)(uint64_t(*)(void))[]()->uint64_t {
		// getFreeMemory
		struct sysinfo info;
		sysinfo(&info);
		return info.freeram*info.mem_unit;
	};
	myvtable[223]=(void*)(uint64_t(*)())[]()->uint64_t {
		// getMemoryLimit
		return 16L*1024L*1024L*1024L;
	};
	myvtable[224]=(void*)(uint64_t(*)())[]()->uint64_t {
		// getUsedMemory
		return 0;
	};
	myvtable[238]=(void*)(bool(*)())[]()->bool {
		// isTablet??
		return false;
	};
	myvtable[265]=(void*)(uint64_t(*)())[]()->uint64_t {
		// calculateAvailableDiskFreeSpace(Core::Path const&)
		return 1024L*1024L*1024L*1024L;
	};
	myvtable[302]=(void*)(bool(*)(void))[]()->bool {
		// canAppSelfTerminate
		return true;
	};
	void *justCurrentWd=(void*)(std::string(*)())[]()->std::string {
		Log::info("Launcher", "current workdir");
		return "";
	};
	myvtable[331]=justCurrentWd;
	myvtable[332]=justCurrentWd;
	myvtable[333]=justCurrentWd;
	myvtable[334]=justCurrentWd;
	myvtable[342]=justCurrentWd;
	*(void**)AppPlatform_obj=myvtable;
	std::string (*myGetInternalStorageFunc)()=[]()->std::string {
		Log::info("Launcher", "test");
		return "";
	};
	int (*myStub)()=[]()->int {
		Log::info("Launcher", "stub!");
		return 0;
	};
	myvtable[70]=(void*)myStub;
	uint64_t jump_val[2]={0xD61F012058000049L};
	jump_val[1]=(uint64_t)myStub;
	memcpy((void*)(base+0x5CC015C), (void*)jump_val, 16); // android stub
	*(uint64_t*)(base+0xDfc5a58)=(uint64_t)myStub;
	*(uint64_t*)(base+0xdfc5a60)=(uint64_t)myStub;
	*(uint64_t*)(base+0xdfc5a68)=(uint64_t)myStub;
	// ^ Bedrock::StorageArea_android hooks
	jump_val[1]=(uint64_t)logHook;
	memcpy((void*)(base+0xD3E23E8), (void*)jump_val, 16);
	// Dirty hook, disabling GameRules copy bc it crashes at Level::initialize
	jump_val[1]=base+0xB78F5AC;
	memcpy((void*)(base+0xB793B04), (void*)jump_val, 16);
	{
		size_t base;
		size_t size;
		linker::get_library_code_region(handle,base,size);
		Log::debug("Launcher", "Protecting Minecraft");
		if(mprotect((void*)base,size,PROT_READ|PROT_EXEC)!=0) {
			perror("mprotect");
			Log::error("Launcher", "Unable to protect Minecraft");
			_Exit(1);
		}
		Log::debug("Launcher", "Minecraft is now protected");
	}
	Log::info("Launcher", "Initializing AppPlatform");
	AppPlatform_obj->initialize();
	//uint64_t platformRuntimeInformation=((uint64_t(*)(void*))(base+0x5C138FC))(AppPlatform_obj);
	//std::string *platformInfoStr=(std::string*)(platformRuntimeInformation+8);
	//(*platformInfoStr)="Linux Dedicated Server 12345678 12345678";
	/*void (*MinecraftWorkerPool_loadWorkerConfigurations)(uint64_t,uint64_t)=(void(*)(uint64_t,uint64_t))(base+0xC5B6A8C);
	void (*MinecraftWorkerPool_createSingletons)(void)=(void(*)(void))(base+0xC5B6D78);
	void (*MinecraftWorkerPool_configureMainThread)(void)=(void(*)(void))(base+0xC5B6C70);
	Log::info("Launcher", "Initializing MinecraftWorkerPool");
	MinecraftWorkerPool_loadWorkerConfigurations(8,8);
	MinecraftWorkerPool_configureMainThread();
	MinecraftWorkerPool_createSingletons();*/
	Log::info("Launcher", "Constructing MinecraftEventing");
	Core::PathBuffer emptyPathBuffer("");
	MinecraftEventing *MinecraftEventing_obj=new MinecraftEventing(emptyPathBuffer);
	//Core::Path currentPath{currentWd,true,strlen(currentWd)};
	Core::Path currentPath{""};
	Log::debug("Launcher", "Initializing MinecraftEventing");
	MinecraftEventing_obj->init(*AppPlatform_obj);
	//WorldSessionEndPoint endPoint(*MinecraftEventing_obj);
	Log::debug("Launcher", "Creating Core::FilePathManager");
	Core::FilePathManager FilePathManager_object(currentPath,true);
	Log::debug("Launcher", "Creating SaveTransactionManager");
	WorkerPool *asyncWorkerPool=(WorkerPool*)(base+0xE21C918);
	SaveTransactionManager *stm_obj=new SaveTransactionManager(*asyncWorkerPool, *MinecraftScheduler::client(), [](bool saving) {
		if(saving) {
			Log::debug("SaveTransactionManager", "Saving...");
		}else{
			Log::debug("SaveTransactionManager", "Saved");
		}
	});
	Log::debug("Launcher", "Creating ExternalFileLevelStorageSource");
	//ExternalFileLevelStorageSource *eflss_obj_ptr=new ExternalFileLevelStorageSource(FilePathManager_object,*stm_obj);
	//ExternalFileLevelStorageSource &eflss_obj=*eflss_obj_ptr;
	ExternalFileLevelStorageSource eflss_obj(FilePathManager_object,*stm_obj);
	//void *refs[4]={0};
	//refs[0]=(void*)&FilePathManager_object;
	//refs[2]=(void*)&stm_obj;
	properties::property<std::string> world_dir(prop, "level-dir", "world");
	Log::debug("Launcher", "Creating ContentTierManager");
	ContentTierManager ctm_obj([]()->bool {
		Log::debug("ContentTierManager", "call stub true");
		return true;
	});
	Log::debug("Launcher", "Creating ResourcePackManager");
	ResourcePackManager *resourcePackManager=new ResourcePackManager([]()->Core::PathBuffer {
		return Core::PathBuffer("");
	}, ctm_obj,false);
	std::shared_ptr<VanillaInPackagePacks> vanillaInPackagePacks=std::make_shared<VanillaInPackagePacks>();
	Log::debug("Launcher", "Creating PackSourceFactory");
	PackSourceFactory *packSourceFactory=new PackSourceFactory(vanillaInPackagePacks);
	char __empty_ref[16]={0};
	void *empty_ref=(void*)__empty_ref;
	PackCapabilityRegistry packCapabilityRegistry(empty_ref);
	Log::debug("Launcher", "Creating PackManifestFactory");
	PackManifestFactory PackManifestFactory_object(packCapabilityRegistry,(void*)((uint64_t)MinecraftEventing_obj+24));
	void *stubContentKeyProviderV[8];
	stubContentKeyProviderV[0]=(void*)myStub;
	stubContentKeyProviderV[1]=(void*)myStub;
	stubContentKeyProviderV[2]=(void*)(std::string(*)())[]()->std::string {
		return "";
	};
	stubContentKeyProviderV[3]=stubContentKeyProviderV[2];
	stubContentKeyProviderV[4]=(void*)myStub;
	stubContentKeyProviderV[5]=(void*)myStub;
	stubContentKeyProviderV[6]=(void*)myStub;
	stubContentKeyProviderV[7]=(void*)myStub;
	StubContentKeyProvider stubContentKeyProvider;
	memset((void*)stubContentKeyProvider.filler,0,0x18-16);
	*(void**)&stubContentKeyProvider=(void*)stubContentKeyProviderV;
	Log::debug("Launcher", "Creating ResourcePackRepository");
	ResourcePackRepository *repo=new ResourcePackRepository(*MinecraftEventing_obj, PackManifestFactory_object, stubContentKeyProvider, FilePathManager_object,*packSourceFactory,false);
	Log::debug("Launcher", "Adding vanilla resource pack");
	std::unique_ptr<ResourcePackStack> stack (new ResourcePackStack());
	Log::debug("Launcher", "vanilla pack is: %p", *((void**)repo+15));
	//std::string const &val=((std::string const&(*)(void*))(base+0x8E0A710))(*((void**)repo+15));
	//raise(SIGTRAP);
	void *packManifest=*(void**)(**(*((uint64_t***)repo +15) +4)+24);
	std::string *vanilla_location=(std::string*)((uint64_t)packManifest+8 +8);
	uint64_t vanilla_size=*((uint64_t*)packManifest +96);
	Log::debug("Launcher", "vanilla pack is at %s, size %ld", vanilla_location->c_str(),vanilla_size);
	stack->add(PackInstance(**((ResourcePack**)repo +15), -1, false, nullptr), *repo, false);
	Log::debug("Launcher", "Added resource pack");
	void (*ResourcePackManager_setStack)(void *,std::unique_ptr<ResourcePackStack>, int, bool)=(void(*)(void*,std::unique_ptr<ResourcePackStack>,int,bool))(base+0x8D8ADB0);
	resourcePackManager->setStack(std::move(stack),4,false);
	Log::debug("Launcher", "set stack");
	ServerInstanceEventCoordinator *serverInstanceEC=new ServerInstanceEventCoordinator;
	LevelDbEnv levelDbEnv;
	/*Log::debug("Launcher", "Creating LevelListCache");
	std::function<bool()> llc_cb=[]()->bool {
		Log::debug("LevelListCache", "callback stubbed true");
		return true;
	};
	auto llc_ptr1=&llc_cb;
	LevelListCache levelListCache(eflss_obj, &llc_ptr1);
	Log::debug("Launcher", "Creating LevelDbEnv");
	Log::debug("Launcher", "Creating FileArchiver");
	static FileArchiver *archiver=new FileArchiver(*MinecraftScheduler::client(), levelListCache, &FilePathManager_object, *repo,false,nullptr,stubContentKeyProvider, levelDbEnv, [](std::string const& str) {
		Log::debug("FileArchiver/callback", "string: %s", str.c_str());
	});*/
	//static VanillaGameModuleApp sharedModule;
	static VanillaGameModuleApp *sharedModule;
	VanillaGameModuleApp::getGameModule(&sharedModule);
	void **minecraftApp=(void**)malloc(60*10);
	memset((void*)minecraftApp,0,600);
	minecraftApp[0]=(void*)myStub;
	minecraftApp[1]=(void *)myStub;
	// ^ destructors
	minecraftApp[2]=(void*)(void(*)())[]() {
		Log::debug("ab","2");
		abort();
	};
	minecraftApp[3]=(void*)(void(*)())[]() {
		Log::debug("ab","3");
		abort();
	};
	minecraftApp[4]=(void*)(void*(*)(void))[]()->void* {
		// getPrimaryMinecraft
		Log::debug("IMinecraftApp::getPrimaryMinecraft", "stubbed");
		return nullptr;
	};
	minecraftApp[5]=(void*)(void*(*)(void))[]()->void* {
		Log::debug("getAutomationClient", "stubbed");
		return nullptr;
	};
	minecraftApp[6]=(void*)(bool(*)(void))[]()->bool {
		Log::debug("isEduMode", "false");
		// isEduMode
		return false;
	};
	minecraftApp[7]=(void*)(bool(*)(void))[]()->bool {
		Log::debug("isDedicatedServer", "true");
		// isDedicatedServer
		return true;
	};
	minecraftApp[8]=(void*)(void(*)(void))[]() {
		Log::debug("onNetworkMaxPlayersChanged", "triggered");
	};
	minecraftApp[9]=(void*)(void*(*)(void))[]()->void* {
		Log::debug("getGameModuleShared", "called");
		return (void*)sharedModule;
	};
	minecraftApp[10]=(void*)(void(*)(std::string const& msg))[](std::string const& msg) {
		Log::debug("requestServerShutdown", "Shutdown requested (stubbed), reason: %s", msg.c_str());
	};
	minecraftApp[11]=(void*)(void*(*)(void))[]()->void* {
		Log::debug("getFileArchiver", "called");
		return nullptr;
	};
	minecraftApp[12]=(void*)(void(*)())[]() {
		Log::debug("ab","12");
		abort();
	};
	minecraftApp[13]=(void*)(void(*)())[]() {
		Log::debug("ab","13");
		abort();
	};
	minecraftApp[14]=(void*)(void(*)())[]() {
		Log::debug("ab","14");
		abort();
	};
	MinecraftApp *mcApp=new MinecraftApp;
	mcApp->vtable=(void*)minecraftApp;
	Log::debug("Launcher", "Creating CodeBuilder::Manager");
	CodeBuilder::Manager manager(*mcApp);
	((ServiceReference(*)(CodeBuilder::Manager *))(base+0x61D746C))(&manager);
	Log::debug("Launcher", "Creating ServerInstance");
	ServerInstance *serverInstance=new ServerInstance(*mcApp, *serverInstanceEC);
	Log::debug("Launcher", "ServerInstance constructed!");
	AllowList allowList;
	PermissionsFile perm;
	perm.reload();
	Log::debug("Launcher", "Creating LevelData");
	LevelData *LevelData_object=new LevelData(false);
	eflss_obj.getLevelData(world_dir, *LevelData_object);
	//*((char*)LevelData_object +1244)=1;
	//*((char*)LevelData_object +1265)=allow_cheats;
	Log::debug("Launcher", "Creating LevelSettings");
	LevelSettings *settings=new LevelSettings();
	unsigned short *version=(unsigned short*)((uint64_t)settings+432);
	version[0]=1;
	version[1]=21;
	version[2]=3;
	//*((char*)version+80)=1;
	// ^ ESSENTIAL, if unset, game will crash for NULL Biome pointers
	std::random_device random_dev{"/dev/urandom"};
	properties::property<std::string> level_seed(prop, "level-seed", std::to_string(random_dev()));
	properties::property<std::string> gamemode_str(prop, "gamemode", "survival");
	char gamemode=0;
	if(gamemode_str.get()=="survival") {
		gamemode=0;
	}else if(gamemode_str.get()=="creative") {
		gamemode=1;
	}else if(gamemode_str.get()=="adventure") {
		gamemode=2;
	}else{
		Log::error("Launcher", "Invalid gamemode specified: %s, allowed values: survival, creative, adventure", gamemode_str.get().c_str());
		_Exit(3);
	}
	properties::property<bool> hardcore(prop, "hardcore", false);
	properties::property<bool> education_features_enabled(prop, "education-features-enabled", false);
	properties::property<bool> allow_cheats(prop, "allow-cheats", false);
	properties::property<int> world_generator(prop, "level-generator", 1);
	properties::property<bool> bonus_chest(prop, "bonus-chest-enabled", false);
	properties::property<bool> start_with_map(prop, "start-with-map-enabled", false);
	properties::property<bool> force_gamemode(prop, "force-gamemode", false);
	properties::property<bool> immutable_world(prop, "immutable-world", false);
	properties::property<std::string> difficulty_str(prop, "difficulty", "easy");
	int difficulty;
	if(difficulty_str.get()=="peaceful") {
		difficulty=0;
	}else if(difficulty_str.get()=="easy") {
		difficulty=1;
	}else if(difficulty_str.get()=="normal") {
		difficulty=2;
	}else if(difficulty_str.get()=="hard") {
		difficulty=3;
	}else{
		Log::error("Launcher", "Invalid difficulty, allowed values: peaceful, easy, normal, hard");
		_Exit(3);
	}
	*((char*)settings+127)=1; // override saved settings
	*(uint64_t*)settings=std::stol(level_seed);
	*((int*)((char*)settings+8))=gamemode;
	*((char*)settings+12)=hardcore;
	*((char*)settings+96)=education_features_enabled;
	*((char*)settings+120)=allow_cheats;
	*((int*)settings+6)=world_generator;
	*((char*)settings+128)=bonus_chest;
	*((char*)settings+129)=start_with_map;
	*((char*)settings+20)=force_gamemode;
	*((char*)settings+93)=immutable_world;
	Log::debug("Launcher", "Creating EducationOptions");
	std::unique_ptr<EducationOptions> eduOpts=std::make_unique<EducationOptions>(resourcePackManager);
	auto pathToLevel=eflss_obj.getPathToLevel(world_dir);
	Log::debug("Launcher", "Path to level is: %s", pathToLevel.path.c_str());
	Log::debug("Launcher", "Getting FileStorageArea");
	std::shared_ptr<Core::FileStorageArea> storageArea;
	Core::FileStorageArea::getStorageAreaForPath(storageArea, Core::Path{pathToLevel.path});
	if(!storageArea) {
		Log::error("Launcher", "NO FileStorageArea");
		_Exit(1);
	}
	auto createLevelStorage=[LevelData_object,&eflss_obj,&stubContentKeyProvider,&levelDbEnv,&world_dir](Scheduler& scheduler)->OwnerPtr<LevelStorage> {
		Log::debug("Launcher", "createLevelStorage");
		// flush interval 12A05F200
		//uint64_t flush_interval=0x12A05F200;
		static std::chrono::nanoseconds flush_interval=std::chrono::minutes(5);
		static ContentIdentity empty_contentIdentity;
		return eflss_obj.createLevelStorage(scheduler, world_dir, empty_contentIdentity, stubContentKeyProvider,flush_interval,levelDbEnv,std::make_unique<LevelStorageEventing>("Starting dedicated server",LevelData_object,"idk what is this"));
		//Log::debug("Launcher", "level storage created");
		//return ptr;
	};
	properties::property<int> compression_threshold(prop, "compression-threshold", 1);
	properties::property<std::string> compression_algorithm(prop, "compression-algorithm", "zlib");
	NetworkSettingOptions networkOpts {
		(unsigned short)compression_threshold, 0, false, 0, 0.0
	};
	if(compression_algorithm.get()=="snappy")
		networkOpts.CompressionAlgorithm=1;
	else if(compression_algorithm.get()!="zlib")
		Log::warn("Launcher", "compression-algorithm: allowed values: zlib, snappy; falling back to default - zlib");
	NetworkPermissions networkPerms;
	NetworkSessionOwner networkSessionOwner;
	networkSessionOwner.createNetworkSession(0);
	cereal::ReflectionCtx &reflectionCtx=cereal::ReflectionCtx::global();
	properties::property<std::string> world_name(prop, "level-name", "Bedrock level");
	properties::property<std::string> motd(prop, "server-name", "arm64 Bedrock Dedicated Server");
	properties::property<int> max_view_distance(prop, "view-distance", 32);
	properties::property<int> server_port(prop, "server-port", 19132);
	properties::property<int> server_port_v6(prop, "server-portv6", 19133);
	properties::property<int> max_players(prop, "max-players", 20);
	properties::property<bool> online_mode(prop, "online-mode", true);
	properties::property<float> player_idle_timeout(prop, "player-idle-timeout", 0.f);
	properties::property<std::string> language(prop, "language", "en_US");
	Log::debug("Launcher", "Initializing ServerInstance");
	bool succ=serverInstance->initializeServer(*mcApp, allowList, &perm, 
			FilePathManager_object,
			std::chrono::minutes(player_idle_timeout), world_dir, world_name, 
			motd, *settings, max_view_distance, true,
			{(uint16_t)server_port, (uint16_t)server_port_v6, max_players},
			online_mode, {}, "normal", *(mce::UUID*)(base+0xE275AE8),
			*MinecraftEventing_obj, *repo, ctm_obj, *resourcePackManager, 
			createLevelStorage, "worlds", nullptr, "", "", "", "", "",
			std::move(eduOpts), resourcePackManager, []() {
			Log::info("Server", "Unloading level");
		}, []() {
			Log::info("Server", "Saving level");
		}, nullptr, nullptr, false, storageArea, networkOpts, false, false, false,std::nullopt, std::nullopt, *(Experiments*)((uint64_t)settings+376), false, 0.0, std::nullopt, ForceBlockNetworkIdsAreHashes::UseDefault, networkPerms, networkSessionOwner, nullptr, reflectionCtx, nullptr);
	if(!succ) {
		Log::error("Launcher", "Failed to initialize ServerInstance");
		_Exit(1);
	}
	Log::debug("Launcher", "ServerInstance initialized!!");
	*((uint64_t*)serverInstance +66)=10;
	uint64_t *I18n_obj=((uint64_t *(*)())(base+0x918AF68))();
	(*(void(**)(void*,ResourcePackManager*))(*I18n_obj+48))(I18n_obj,resourcePackManager);
	(*(void(**)(void*,std::string const&))(*I18n_obj+136))(I18n_obj, language);
	Log::info("Launcher", "Starting server");
	serverInstance->startServerThread();
	Log::info("Launcher", "Server started!");
	ConsoleReader reader;
	ConsoleReader::registerInterruptHandler();

	std::string line;
	while(reader.read(line)) {
		serverInstance->queueForServerThread([&serverInstance, line]() {
			std::unique_ptr<ServerCommandOrigin> commandOrigin(new ServerCommandOrigin("Server", (ServerLevel &)*serverInstance->_getMinecraft()->getLevel(), 5, true));
			serverInstance->_getMinecraft()->_getCommands()->requestCommandExecution(std::move(commandOrigin), line, 4, true);
		});
	}

	Log::info("Launcher", "Stopping...");
	serverInstance->leaveGameSync();
	Log::info("Launcher", "exit");
	_Exit(0);
	return 0;
}

void printVersionInfo() {
	printf("arm64-mcpelauncher-server\n");
}

