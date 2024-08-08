#pragma once
#include <string>
#include <unordered_map>


uintptr_t base;

namespace Bedrock {
	class EnableNonOwnerReferences {
	public:
		struct ControlBlock {
			EnableNonOwnerReferences *ptr;
		};

		std::shared_ptr<ControlBlock> controlBlock;

		virtual ~EnableNonOwnerReferences() {}
		EnableNonOwnerReferences() {
			controlBlock=std::make_shared<ControlBlock>();
			controlBlock->ptr=this;
		}

		EnableNonOwnerReferences(Bedrock::EnableNonOwnerReferences const& ref) {
			controlBlock=ref.controlBlock;
		}
	};

	template <typename T>
	class NonOwnerPointer {
	public:
		std::shared_ptr<Bedrock::EnableNonOwnerReferences::ControlBlock> controlBlock;
		NonOwnerPointer(std::nullptr_t) noexcept {}
		NonOwnerPointer(T const& orig) {
			controlBlock=orig.controlBlock;
		}
	};
};

template <typename T>
class OwnerPtr {
public:
	std::shared_ptr<T> ptr;
	OwnerPtr(T* item) {
		ptr=std::shared_ptr<T>(item);
	}
};

class ServiceReference {
	char filler[0x16];
};

class Scheduler;
class MinecraftScheduler {
public:
	static Scheduler *client() {
		return ((Scheduler*(*)(void))(base+0xCE33634))();
	}
};

class WorkerPool;
class SaveTransactionManager:public Bedrock::EnableNonOwnerReferences {
	char filler[0x150-16];
public:
	SaveTransactionManager(WorkerPool &wp,Scheduler &sch,std::function<void(bool)> func) {
		((void(*)(SaveTransactionManager*,WorkerPool&,Scheduler&,std::function<void(bool)>))(base+0x9449BA8))(this,wp,sch,func);
	}
};

namespace Core {
	struct Path {
		std::string path;
		/*const char* path;
		bool hasSize;
		size_t size;*/
	};

	struct PathBuffer {
		std::string path;
		//size_t size;

		PathBuffer(std::string const& str) {
			path = str;
			//size = str.length();
		}
	};

	struct Result {
		char unsurefiller[0x18];
	};

	struct FileStorageArea {
		static Result getStorageAreaForPath(std::shared_ptr<FileStorageArea>& ptr, Path const& path) {
			return ((Result(*)(std::shared_ptr<FileStorageArea>&, Path const&))(base+0xD3E86B0))(ptr,path);
		}
	};

	struct FilePathManager:public Bedrock::EnableNonOwnerReferences {
		char filler[0xC8-16];
		FilePathManager(Path const& path, bool v) {
			memset((void*)filler,0,0xc8-16);
			((void(*)(FilePathManager*,Path const&, bool))(base+0xD40E840))(this,path,v);
		}
	};
};

struct ContentIdentity {
	int64_t a;
	int64_t b;
	int64_t c;
	ContentIdentity() {
		a=0;
		b=0;
		c=0;
	}
};

struct LevelDbEnv:public Bedrock::EnableNonOwnerReferences {
	char filler[0x28-16];
	LevelDbEnv() {
		((void(*)(LevelDbEnv*))(base+0x9187960))(this);
	}
};

struct LevelStorageEventing {
	char filler[0x58];
	LevelStorageEventing(std::string const& a, void* b, std::string const& c) {
		((void(*)(void*,std::string const&, void *, std::string const&))(base+0xB7A6C2C))((void*)this,a,b,c);
	}
};

struct StubContentKeyProvider : public Bedrock::EnableNonOwnerReferences {
	char filler[0x18-16];
	StubContentKeyProvider() {}
};


class LevelStorage;
class LevelData;

class ExternalFileLevelStorageSource {
	char filler[0x38];
public:
	ExternalFileLevelStorageSource(Bedrock::NonOwnerPointer<Core::FilePathManager> const& fpm, Bedrock::NonOwnerPointer<SaveTransactionManager> const& stm) {
		((void(*)(ExternalFileLevelStorageSource*,Bedrock::NonOwnerPointer<Core::FilePathManager> const&, Bedrock::NonOwnerPointer<SaveTransactionManager> const&))(base+0xB78CB8C))(this,fpm,stm);
	}

	OwnerPtr<LevelStorage> createLevelStorage(Scheduler& scheduler, std::string const& name, ContentIdentity const& contentIdentity, Bedrock::NonOwnerPointer<StubContentKeyProvider const> const& contentKeyProvider, std::chrono::nanoseconds const& flushInterval, Bedrock::NonOwnerPointer<LevelDbEnv> levelDBEnv, std::unique_ptr<LevelStorageEventing> eventing) {
		return ((OwnerPtr<LevelStorage>(*)(ExternalFileLevelStorageSource *, Scheduler&,std::string const&, ContentIdentity const&,Bedrock::NonOwnerPointer<StubContentKeyProvider const> const&, std::chrono::nanoseconds const&, Bedrock::NonOwnerPointer<LevelDbEnv>, std::unique_ptr<LevelStorageEventing>))(base+0xB78CF6C))(this,scheduler,name,contentIdentity, contentKeyProvider, flushInterval, levelDBEnv, std::move(eventing));
	}

	Core::PathBuffer getPathToLevel(std::string const& level) {
		return ((Core::PathBuffer(*)(ExternalFileLevelStorageSource *,std::string const&))(base+0xB78EDE4))(this,level);
	}

	Core::Result getLevelData(std::string const& level, LevelData &data) {
		return ((Core::Result(*)(ExternalFileLevelStorageSource *,std::string const&,LevelData&))(base+0xB78E89C))(this,level,data);
	}
};

class ConnectionDefinition {
	short port, portv6;
	int unk1;
	int maxPlayers;
	int unk2 = 0;
public:
	ConnectionDefinition(int port, int portv6, int max) : port((short) port), portv6((short) portv6), maxPlayers(max) {}
};

struct LevelData {
	char filler[1600];
	LevelData(bool isEduMode) {
		((void(*)(LevelData*,bool))(base+0xB798A60))(this,isEduMode);
	}
};

class ContentTierManager:public Bedrock::EnableNonOwnerReferences {
	char filler[128-16];
public:
	ContentTierManager(std::function<bool(void)> func) {
		((void(*)(ContentTierManager*,std::function<bool(void)>))(base+0x8FB4550))(this,func);
	}
};

class ServerInstanceEventCoordinator:public Bedrock::EnableNonOwnerReferences {
	char filler[0x80-16];
public:
	ServerInstanceEventCoordinator() {
		memset((void*)filler,0,0x80-16);
		*(uint64_t*)this=base+0xDaB8310;
	};
};

struct ResourcePackStack;
class ResourcePackManager {
	char filler[0x1C0];
public:
	ResourcePackManager(std::function<Core::PathBuffer(void)> func,Bedrock::NonOwnerPointer<ContentTierManager const> const& ctm, bool b1) {
		((void(*)(ResourcePackManager*,std::function<Core::PathBuffer(void)>,Bedrock::NonOwnerPointer<ContentTierManager const> const&, bool))(base+0x8F6A0EC))(this,func,ctm,b1);
	}
	void setStack(std::unique_ptr<ResourcePackStack> stack, int a1, bool a2) {
		return ((void(*)(ResourcePackManager*,std::unique_ptr<ResourcePackStack>,int,bool))(base+0x8F6F3B4))(this,std::move(stack),a1,a2);
	}
};

/*struct LevelListCache {
	char filler[0x100];
	// I can't do std::function<...> && so let's do **
	LevelListCache(ExternalFileLevelStorageSource &source, std::function<bool()> ** func) {
		((void(*)(LevelListCache*,ExternalFileLevelStorageSource &,std::function<bool()> **))(base+0xB62C078))(this,source,func);
	}
};*/

struct MinecraftApp {
	void *vtable;
};

class CodeBuilder {
public:
	class Manager {
		char filler[0x28];
	public:
		Manager(MinecraftApp& app) {
			((void(*)(CodeBuilder::Manager*,MinecraftApp&))(base+0xC6D490C))(this,app);
		}
	};
};

struct VanillaInPackagePacks {
	void *vtable;
	char filler[0x20-8];
	VanillaInPackagePacks() {
		vtable=(void*)(base+0xDA113F8);
		memset((void*)filler,0,0x20-8);
	}
};

class PackSourceFactory {
	char filler[0x1f8];
public:
	PackSourceFactory(std::shared_ptr<VanillaInPackagePacks> const& packs) {
		((void(*)(PackSourceFactory*,std::shared_ptr<VanillaInPackagePacks> const&))(base+0x8F5E52c))(this,packs);
	}
};

struct PackCapabilityRegistry {
	char filler[16];
	PackCapabilityRegistry(void *ref) {
		((void(*)(PackCapabilityRegistry*,void*))(base+0x8FBFC88))(this,ref);
	}
};

struct PackManifestFactory {
	char filler[0x28];
	PackManifestFactory(PackCapabilityRegistry const& pcr, void *telemetry) {
		((void(*)(PackManifestFactory*,PackCapabilityRegistry const&, void *))(base+0x8f4c170))(this,pcr,telemetry);
	}
};

struct MinecraftEventing;
class ResourcePackRepository : public Bedrock::EnableNonOwnerReferences {
	char filler[0x220-16];
public:
	ResourcePackRepository(MinecraftEventing& a,PackManifestFactory& b,Bedrock::NonOwnerPointer<StubContentKeyProvider> const& c,Bedrock::NonOwnerPointer<Core::FilePathManager> const& d,PackSourceFactory& e,bool f) {
		((void(*)(ResourcePackRepository*,MinecraftEventing&,PackManifestFactory&,Bedrock::NonOwnerPointer<StubContentKeyProvider> const&,Bedrock::NonOwnerPointer<Core::FilePathManager> const&,PackSourceFactory&,bool))(base+0x8F750C4))(this,a,b,c,d,e,f);
	}
};

class VanillaGameModuleServer {
	char filler[40];
public:
	VanillaGameModuleServer() {
		((void(*)(VanillaGameModuleServer*))(base+0x5AECB3C))(this);
	}
};

class VanillaGameModuleApp {
	char filler[32];
public:
	//VanillaGameModuleApp() {
	//	((void(*)(VanillaGameModuleApp*))(base+0x4B9AB0C))(this);
	//}
	static void getGameModule(VanillaGameModuleApp **val) {
		((void(*)(VanillaGameModuleApp **))(base+0x4b9ab0c))(val);
	}
};

/*class FileArchiver:public Bedrock::EnableNonOwnerReferences {
	class IWorldConverter {};

	char filler[0x1E06];
public:
	FileArchiver(Scheduler& scheduler, LevelListCache &llc, Core::FilePathManager *fpm, Bedrock::NonOwnerPointer<ResourcePackRepository> const& repo, bool idk1, std::unique_ptr<FileArchiver::IWorldConverter> idk2, Bedrock::NonOwnerPointer<StubContentKeyProvider const> keyProvider, Bedrock::NonOwnerPointer<LevelDbEnv> levelDbEnv, std::function<void (std::string const&)> func) {
		((void(*)(FileArchiver*,Scheduler&,LevelListCache &,Core::FilePathManager*, Bedrock::NonOwnerPointer<ResourcePackRepository> const&, bool, std::unique_ptr<FileArchiver::IWorldConverter>, Bedrock::NonOwnerPointer<StubContentKeyProvider const>, Bedrock::NonOwnerPointer<LevelDbEnv>, std::function<void (std::string const&)>))(base+0xAFEFE6C))(this,scheduler,llc,fpm,repo,idk1,std::move(idk2),keyProvider,levelDbEnv,func);
	}
};*/

class AppConfigs {};
class AppConfigsFactory {
public:
	static std::unique_ptr<AppConfigs> createAppConfigs() {
		return std::move(
			((std::unique_ptr<AppConfigs>(*)())(base+0x922DFA0))()
		);
	}
};

struct AppPlatform : public Bedrock::EnableNonOwnerReferences {
	char filler[0x620-16];
	AppPlatform(bool val) {
		memset((void*)filler,0,0x620-16);
		((void(*)(AppPlatform*,bool))(base+0x5CA95A8))(this,val);
	}

	void initialize() {
		((void(*)(AppPlatform*))(base+0x5CAADF0))(this);
	}
};

struct MinecraftEventing : public Bedrock::EnableNonOwnerReferences {
	char filler[0x1C8-16];
	MinecraftEventing(Core::PathBuffer buf) {
		memset((void*)filler,0,0x1c8-16);
		((void(*)(MinecraftEventing*,Core::PathBuffer))(base+0xC25904C))(this,buf);
	}

	void init(Bedrock::NonOwnerPointer<AppPlatform> const& platform) {
		((void(*)(MinecraftEventing*,Bedrock::NonOwnerPointer<AppPlatform> const&))(base+0xC25966C))(this,platform);
	}
};


struct EducationOptions {
	char filler[0x38];
	EducationOptions(ResourcePackManager *resourcePackManager) {
		((void(*)(void*,ResourcePackManager*))(base+0x8F9E6B4))((void*)this, resourcePackManager);
	}
};

struct PackSettings;
struct ResourcePack:public Bedrock::EnableNonOwnerReferences {
	// tbd: filler
};
struct PackInstance {
	char filler[500];
	PackInstance(Bedrock::NonOwnerPointer<ResourcePack> const& rp, int a2, bool a3, PackSettings *a4) {
		((void(*)(void*,Bedrock::NonOwnerPointer<ResourcePack> const&,int,bool,PackSettings*))(base+0x8EDE8C0))((void*)this, rp,a2,a3,a4);
	}
};

struct ResourcePackStack {
	uint64_t vtable;
	char filler[0x28-8];
	ResourcePackStack() {
		vtable=base+0xDC9F740;
		memset((void*)filler,0,0x28-8);
	}

	void add(PackInstance inst,Bedrock::NonOwnerPointer<ResourcePackRepository> const& res_repo,bool a3) {
		((void(*)(void*,PackInstance,Bedrock::NonOwnerPointer<ResourcePackRepository> const&,bool))(base+0x8EE2954))((void*)this,inst,res_repo,a3);
	}
};

struct PermissionsFile {
	std::string fn="permissions.json";
	char filler[0x50];
	PermissionsFile() {
		memset((void*)filler,0,0x50);
		*((int*)this +14)=1065353216;
	}
	void reload() {
		((void(*)(PermissionsFile*))(base+0x972F830))(this);
	}
};

struct AllowList {
	char filler[0x50];
	AllowList() {
		memset((void*)filler,0,0x50);
		*(uint64_t*)filler=base+0xDCF88A8;
	}
};

struct GameRules {
	char filler[180];
	GameRules() {
		((void(*)(GameRules*))(base+0xB78F5AC))(this);
	}
};

struct LevelSettings {
	char filler[0x468];
	LevelSettings() {
		((void(*)(LevelSettings*))(base+0xB100D80))(this);
	}
};

namespace mce {
	struct UUID {
		char filler[0x16];
	};
};

struct NetworkSettingOptions {
	uint16_t CompressionThresholdBytesize;
	uint16_t CompressionAlgorithm;
	bool ClientThrottleEnabled;
	char ClientThrottleThreshold;
	float ClientThrottleScalar;
};

enum class ForceBlockNetworkIdsAreHashes : char {
	UseDefault = 0x0,
	ForceOff   = 0x1,
	ForceOn    = 0x2,
};

struct NetworkPermissions {
	char filler[64]={0};
};

struct NetworkSessionOwner:public Bedrock::EnableNonOwnerReferences {
	char filler[0x20-16];

	void createNetworkSession(int transportLayer) {
		((void(*)(NetworkSessionOwner*,int))(base+0x9157AB4))(this,transportLayer);
	}
	NetworkSessionOwner() {
		((void(*)(NetworkSessionOwner*))(base+0x91583D4))(this);
	}
};

namespace cereal {
	struct ReflectionCtx {
		static ReflectionCtx& global() {
			return ((ReflectionCtx&(*)(void))(base+0xD84A2BC))();
		}
	};
};

/*struct WorldSessionEndPoint {
	char filler[0x30];
	WorldSessionEndPoint(MinecraftEventing &ev) {
		((void(*)(WorldSessionEndPoint*,MinecraftEventing&))(base+0xAA71298))(this,ev);
	}
};*/

class ContentLog {
	char filler[0xB0];
public:
	ContentLog() {
		((void(*)(ContentLog*))(base+0xD3E30E8))(this);
	}
};

struct Experiments;
struct PlayerMovementSettings {
	char filler[65];
};
struct ScriptSettings {
	char filler[360];
};
struct CDNConfig {
	char filler[40];
};
struct ServerTextSettings {};
class ServerMetrics;
class DebugEndPoint;

class ServerLevel;
class ServerCommandOrigin {
	char filler[0x40];
public:
	ServerCommandOrigin(std::string const& name, ServerLevel& lvl, char perm, int dimension) {
		((void(*)(ServerCommandOrigin*,std::string const&,ServerLevel&,char,int))(base+0x96E9CF4))(this,name,lvl,perm,dimension);
	}
};

class MinecraftCommands {
public:
	void requestCommandExecution(std::unique_ptr<ServerCommandOrigin> origin, std::string const& cmd, int p, bool t) {
		((void(*)(MinecraftCommands*,std::unique_ptr<ServerCommandOrigin>, std::string const&, int, bool))(base+0x95FD068))(this,std::move(origin),cmd,p,t);
	}
};

class Minecraft {
public:
	ServerLevel *getLevel() {
		return ((ServerLevel*(*)(Minecraft*))(base+0xAB8C7D4))(this);
	}

	MinecraftCommands *_getCommands() {
		return *(MinecraftCommands**)((void**)this +22);
	}
};

class ServerInstance {
	char filler[0x2E0];
public:
	ServerInstance(MinecraftApp& app,Bedrock::NonOwnerPointer<ServerInstanceEventCoordinator> const& evc) {
		((void(*)(ServerInstance*,MinecraftApp&,Bedrock::NonOwnerPointer<ServerInstanceEventCoordinator> const&))(base+0x9731948))(this,app,evc);
	}

	bool initializeServer(MinecraftApp& app, AllowList& allowList, PermissionsFile* perm, 
			Bedrock::NonOwnerPointer<Core::FilePathManager> const& fpm, 
			std::chrono::seconds idleTimeout, 
			std::string worldName, 
			std::string displayName, 
			std::string serverMotd, 
			LevelSettings levelSettings, 
			int maximumViewDistance, 
			bool idk_use_true, 
			ConnectionDefinition const& connDef, // differ from def
			bool onlineMode, 
			std::vector<std::string> const& idk_use_empty, 
			std::string idk_use_normal, 
			mce::UUID const& someUUID, MinecraftEventing& ev,
			Bedrock::NonOwnerPointer<ResourcePackRepository> const& resRepo, 
			Bedrock::NonOwnerPointer<ContentTierManager const> const& ctm, 
			ResourcePackManager& resMgr, 
			std::function<OwnerPtr<LevelStorage>(Scheduler&)> createLevelStorage,
		       	std::string const& worldsPath, 
			LevelData* levelDat, 
			std::string idk1, 
			std::string idk2, 
			std::string idk3, 
			std::string idk4, 
			std::string idk5,
			std::unique_ptr<EducationOptions> eduOptions, 
			ResourcePackManager* resMgrPtr,
		       	std::function<void()> unloadLevelFunc, 
			std::function<void()> savingLevelFunc, 
			ServerMetrics* serverMetrics, DebugEndPoint* dbgEndPoint, 
			bool idk_use_false, 
			std::shared_ptr<Core::FileStorageArea> storageArea, 
			NetworkSettingOptions const& networkSettingOpts, 
			bool idk6, bool idk7, bool idk8, 
			std::optional<PlayerMovementSettings> playerMovementSettings, 
			std::optional<ScriptSettings> scriptSettings, 
			Experiments const& exp, 
			bool idk9, 
			float idk10, 
			std::optional<bool> idk11, 
			ForceBlockNetworkIdsAreHashes blockNetworkIdsAreHashes, 
			NetworkPermissions const& networkPerms, 
			Bedrock::NonOwnerPointer<NetworkSessionOwner> networkSessionOwner, 
			Bedrock::NonOwnerPointer<CDNConfig> cdnConfig, //nullable
			cereal::ReflectionCtx& reflectionCtx, 
			Bedrock::NonOwnerPointer<ServerTextSettings> textSettings/*nullable*/) {
		return ((bool(*)(ServerInstance*,MinecraftApp&, AllowList&, PermissionsFile*, Bedrock::NonOwnerPointer<Core::FilePathManager> const&, std::chrono::seconds, std::string, std::string, std::string, LevelSettings, int, bool, ConnectionDefinition const&, bool, std::vector<std::string> const&, std::string, mce::UUID const&, MinecraftEventing&, Bedrock::NonOwnerPointer<ResourcePackRepository> const&, Bedrock::NonOwnerPointer<ContentTierManager const> const&, ResourcePackManager&, std::function<OwnerPtr<LevelStorage>(Scheduler&)>, std::string const&, LevelData*, std::string, std::string, std::string, std::string, std::string, std::unique_ptr<EducationOptions>, ResourcePackManager*, std::function<void()>, std::function<void()>, ServerMetrics*, DebugEndPoint*, bool, std::shared_ptr<Core::FileStorageArea>, NetworkSettingOptions const&, bool, bool, bool, std::optional<PlayerMovementSettings>, std::optional<ScriptSettings>, Experiments const&, bool, float, std::optional<bool>, ForceBlockNetworkIdsAreHashes, NetworkPermissions const&, Bedrock::NonOwnerPointer<NetworkSessionOwner>, Bedrock::NonOwnerPointer<CDNConfig>, cereal::ReflectionCtx&, Bedrock::NonOwnerPointer<ServerTextSettings>
))(base+0x9731fc0))(this,app,allowList,perm,fpm,idleTimeout,worldName,displayName,serverMotd,levelSettings,maximumViewDistance,idk_use_true,connDef,onlineMode,idk_use_empty,idk_use_normal,someUUID,ev,resRepo,ctm,resMgr,createLevelStorage,worldsPath,levelDat,idk1,idk2,idk3,idk4,idk5,std::move(eduOptions),resMgrPtr,unloadLevelFunc,savingLevelFunc,serverMetrics,dbgEndPoint,idk_use_false,storageArea,networkSettingOpts,idk6,idk7,idk8,playerMovementSettings,scriptSettings,exp,idk9,idk10,idk11,blockNetworkIdsAreHashes,networkPerms,networkSessionOwner,cdnConfig,reflectionCtx,textSettings);
	}

	void startServerThread() {
		return ((void(*)(ServerInstance*))(base+0x973A790))(this);
	}

	void queueForServerThread(std::function<void()> func) {
		return ((void(*)(ServerInstance*,std::function<void()>))(base+0x973A970))(this, func);
	}

	void leaveGameSync() {
		return ((void(*)(ServerInstance*))(base+0x973A140))(this);
	}

	inline Minecraft *_getMinecraft() {
		return *(Minecraft**)((uint64_t)this+128);
	}
};





void logHook(unsigned int category, std::bitset<3> set, int rule, int area, unsigned int level, char const* tag, int tid, char const* format, va_list args) {
	LogLevel ourLevel = LogLevel::LOG_ERROR;
	if (level == 1)
		ourLevel = LogLevel::LOG_TRACE;
	if (level == 2)
		ourLevel = LogLevel::LOG_INFO;
	if (level == 4)
		ourLevel = LogLevel::LOG_WARN;
	if (level == 8)
		ourLevel = LogLevel::LOG_ERROR;
	std::string ourTag(((const char*(*)(int))(base+0xD3DDA08))(area));
	ourTag += '/';
	ourTag += tag;
	Log::vlog(ourLevel, ourTag.c_str(), format, args);
}


