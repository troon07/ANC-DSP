#define NOMINMAX
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <ksmedia.h>
#include <propvarutil.h>
#include <thread>
#include <atomic>
#include <array>
#include <utility>
#include "wasapi_backend.h"
namespace anc {
template<class T> struct Com { T* p=nullptr; ~Com(){if(p)p->Release();} T** address(){return &p;} T* operator->()const{return p;} };
static std::string utf8(const wchar_t* s){int n=WideCharToMultiByte(CP_UTF8,0,s,-1,nullptr,0,nullptr,nullptr);std::string r(n,'\0');WideCharToMultiByte(CP_UTF8,0,s,-1,r.data(),n,nullptr,nullptr);r.pop_back();return r;}
static std::wstring wide(const std::string&s){int n=MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,nullptr,0);std::wstring r(n,L'\0');MultiByteToWideChar(CP_UTF8,0,s.c_str(),-1,r.data(),n);r.pop_back();return r;}
static const GUID FloatFormat={3,0,0x0010,{0x80,0,0,0xaa,0,0x38,0x9b,0x71}};
static bool floating(const WAVEFORMATEX* f){return f->wFormatTag==WAVE_FORMAT_IEEE_FLOAT || (f->wFormatTag==WAVE_FORMAT_EXTENSIBLE && IsEqualGUID(reinterpret_cast<const WAVEFORMATEXTENSIBLE*>(f)->SubFormat,FloatFormat));}
static bool supported(const WAVEFORMATEX*f){return floating(f)&&f->wBitsPerSample==32&&f->nChannels>=2&&f->nChannels<=16;}
static HRESULT initializeNative(IAudioClient* client,const WAVEFORMATEX* format) {
    Com<IAudioClient3> modern;
    if(SUCCEEDED(client->QueryInterface(__uuidof(IAudioClient3),reinterpret_cast<void**>(modern.address())))) {
        UINT32 standard=0,fundamental=0,minimum=0,maximum=0;
        if(SUCCEEDED(modern->GetSharedModeEnginePeriod(format,&standard,&fundamental,&minimum,&maximum)))
            return modern->InitializeSharedAudioStream(0,minimum,format,nullptr);
    }
    return client->Initialize(AUDCLNT_SHAREMODE_SHARED,0,200000,0,format,nullptr);
}
struct WasapiBackend::Impl {
    AudioCallback callback;
    bool tonalMode=false;
    std::thread worker; std::atomic<bool> running{false},ready{false};std::atomic<size_t> underruns{0};
    uint32_t rate=0;size_t block=0;float estimatedMs=0;
    std::vector<AudioDeviceInfo> enumerate(EDataFlow flow) {
        CoInitializeEx(nullptr,COINIT_MULTITHREADED); std::vector<AudioDeviceInfo> list;
        {Com<IMMDeviceEnumerator> e;Com<IMMDeviceCollection> collection;
        if(SUCCEEDED(CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,__uuidof(IMMDeviceEnumerator),reinterpret_cast<void**>(e.address()))) && SUCCEEDED(e->EnumAudioEndpoints(flow,DEVICE_STATE_ACTIVE,collection.address()))) {
            UINT count=0;collection->GetCount(&count);for(UINT i=0;i<count;++i){Com<IMMDevice>d;Com<IPropertyStore>props;Com<IAudioClient>client;LPWSTR id=nullptr;PROPVARIANT value;PropVariantInit(&value);
                if(FAILED(collection->Item(i,d.address()))||FAILED(d->GetId(&id)))continue;
                AudioDeviceInfo info;info.id=utf8(id);CoTaskMemFree(id);info.name=info.id;
                if(SUCCEEDED(d->OpenPropertyStore(STGM_READ,props.address()))&&SUCCEEDED(props->GetValue(PKEY_Device_FriendlyName,&value))&&value.vt==VT_LPWSTR)info.name=utf8(value.pwszVal);
                PropVariantClear(&value);
                if(SUCCEEDED(d->Activate(__uuidof(IAudioClient),CLSCTX_ALL,nullptr,reinterpret_cast<void**>(client.address())))){WAVEFORMATEX*f=nullptr;if(SUCCEEDED(client->GetMixFormat(&f))){info.default_sample_rate=f->nSamplesPerSec;info.channels=f->nChannels;CoTaskMemFree(f);}}
                list.push_back(info);
            }
        }}CoUninitialize();return list;
    }
    void run(std::string captureId,std::string renderId,uint32_t requested) {
        CoInitializeEx(nullptr,COINIT_MULTITHREADED);
        {Com<IMMDeviceEnumerator>e;Com<IMMDevice>input,output;Com<IAudioClient>ci,co;Com<IAudioCaptureClient>capture;Com<IAudioRenderClient>render;
        WAVEFORMATEX* fi=nullptr;WAVEFORMATEX* fo=nullptr;
        auto setup=[&]()->bool {
            if(FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator),nullptr,CLSCTX_ALL,__uuidof(IMMDeviceEnumerator),reinterpret_cast<void**>(e.address()))))return false;
            auto a=wide(captureId),b=wide(renderId);
            if(FAILED(e->GetDevice(a.c_str(),input.address()))||FAILED(e->GetDevice(b.c_str(),output.address())))return false;
            if(FAILED(input->Activate(__uuidof(IAudioClient),CLSCTX_ALL,nullptr,reinterpret_cast<void**>(ci.address())))||FAILED(output->Activate(__uuidof(IAudioClient),CLSCTX_ALL,nullptr,reinterpret_cast<void**>(co.address()))))return false;
            if(FAILED(ci->GetMixFormat(&fi))||FAILED(co->GetMixFormat(&fo)))return false;
            bool inputSupported=supported(fi)||(tonalMode&&floating(fi)&&fi->wBitsPerSample==32&&fi->nChannels==1);
            if(!inputSupported||!supported(fo)||fi->nSamplesPerSec!=requested||fo->nSamplesPerSec!=requested)return false;
            if(tonalMode){
                Com<IAudioClient2> raw;
                if(FAILED(ci->QueryInterface(__uuidof(IAudioClient2),reinterpret_cast<void**>(raw.address()))))return false;
                AudioClientProperties properties{};properties.cbSize=sizeof(properties);properties.eCategory=AudioCategory_Other;properties.Options=AUDCLNT_STREAMOPTIONS_RAW;
                if(FAILED(raw->SetClientProperties(&properties)))return false;
            }
            // Native shared-mode formats; reject resampling and mono channel duplication.
            if(FAILED(initializeNative(ci.p,fi))||FAILED(initializeNative(co.p,fo)))return false;
            if(FAILED(ci->GetService(__uuidof(IAudioCaptureClient),reinterpret_cast<void**>(capture.address())))||FAILED(co->GetService(__uuidof(IAudioRenderClient),reinterpret_cast<void**>(render.address()))))return false;
            UINT32 ni=0,no=0;ci->GetBufferSize(&ni);co->GetBufferSize(&no);rate=requested;block=no;estimatedMs=1000.0f*(ni+no)/rate;
            return SUCCEEDED(ci->Start()) && SUCCEEDED(co->Start());
        };
        bool ok=setup();running.store(ok);ready.store(true);
        constexpr size_t Capacity=65536;std::array<float,Capacity>queue{};size_t read=0,write=0,queued=0;
        std::array<float,8192>in{},out{};bool playbackStarted=false,receivedCapture=false;
        while(running.load()) {
            UINT32 packet=0;
            HRESULT hr=capture->GetNextPacketSize(&packet);if(FAILED(hr)){running=false;break;}
            while(packet && running.load()) {
                BYTE*bytes=nullptr;UINT32 frames=0;DWORD flags=0;
                if(FAILED(capture->GetBuffer(&bytes,&frames,&flags,nullptr,nullptr))){running=false;break;}
                if(frames>4096 || (flags&AUDCLNT_BUFFERFLAGS_TIMESTAMP_ERROR) || (receivedCapture && (flags&AUDCLNT_BUFFERFLAGS_DATA_DISCONTINUITY))) {
                    // Discontinuity invalidates the path timing: stop the calibrated loop.
                    capture->ReleaseBuffer(frames);running=false;++underruns;break;
                }
                receivedCapture=true;
                const float*src=reinterpret_cast<float*>(bytes);
                for(UINT32 k=0;k<frames;++k) {in[2*k]=(flags&AUDCLNT_BUFFERFLAGS_SILENT)?0:src[k*fi->nChannels];in[2*k+1]=(flags&AUDCLNT_BUFFERFLAGS_SILENT)||fi->nChannels<2?0:src[k*fi->nChannels+1];out[2*k]=out[2*k+1]=0;}
                AudioFrame a{in.data(),frames,2,rate},b{out.data(),frames,2,rate};
                if(callback)callback(a,b);
                if(queued+frames>Capacity){running=false;++underruns;}else for(UINT32 k=0;k<frames;++k){queue[write]=out[2*k];write=(write+1)%Capacity;++queued;}
                capture->ReleaseBuffer(frames);if(FAILED(capture->GetNextPacketSize(&packet))){running=false;break;}
            }
            if(!running.load())break;
            UINT32 padding=0,capacity=0;co->GetBufferSize(&capacity);
            if(FAILED(co->GetCurrentPadding(&padding))){running=false;break;}
            if(playbackStarted && padding==0){running=false;++underruns;break;}
            UINT32 n=UINT32(std::min(queued,size_t(capacity-padding)));
            if(n) {BYTE*bytes=nullptr;if(FAILED(render->GetBuffer(n,&bytes))){running=false;break;}float*dst=reinterpret_cast<float*>(bytes);
                for(UINT32 k=0;k<n;++k){for(WORD ch=0;ch<fo->nChannels;++ch)dst[k*fo->nChannels+ch]=ch==0?queue[read]:0;read=(read+1)%Capacity;--queued;}
                if(FAILED(render->ReleaseBuffer(n,0))){running=false;break;}playbackStarted=true;
            }
            Sleep(1);
        }
        if(ci.p)ci->Stop();if(co.p)co->Stop();if(fi)CoTaskMemFree(fi);if(fo)CoTaskMemFree(fo);
        }CoUninitialize();running=false;ready=true;
    }
};
WasapiBackend::WasapiBackend():p_(new Impl){} WasapiBackend::~WasapiBackend(){closeStream();}
std::vector<AudioDeviceInfo> WasapiBackend::enumerateCaptureDevices(){return p_->enumerate(eCapture);}
std::vector<AudioDeviceInfo> WasapiBackend::enumerateRenderDevices(){return p_->enumerate(eRender);}
bool WasapiBackend::openStream(const std::string&a,const std::string&b,uint32_t rate,size_t){closeStream();p_->ready=false;p_->underruns=0;p_->worker=std::thread([=]{p_->run(a,b,rate);});while(!p_->ready.load())Sleep(1);return p_->running.load();}
void WasapiBackend::closeStream(){p_->running=false;if(p_->worker.joinable())p_->worker.join();}
bool WasapiBackend::isRunning()const{return p_->running.load();}
void WasapiBackend::setCallback(AudioCallback cb){if(!isRunning())p_->callback=std::move(cb);}
void WasapiBackend::setTonalMode(bool enabled){if(!isRunning())p_->tonalMode=enabled;}
size_t WasapiBackend::getUnderrunCount()const{return p_->underruns.load();}
float WasapiBackend::getEstimatedRoundtripLatencyMs()const{return p_->estimatedMs;}
uint32_t WasapiBackend::getActualSampleRate()const{return p_->rate;}
size_t WasapiBackend::getActualBlockSize()const{return p_->block;}
}
