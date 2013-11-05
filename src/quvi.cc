#include <quvi/quvi.h>
#include <iostream>
#include <string>
#include <node.h>
#include <v8.h>
#include <uv.h>

using namespace std;
using namespace v8;


class WorkInfo {
private:
	Persistent<Function> callback;
	uv_work_t request;

	quvi_media_t media;
	quvi_t q;

	char *strerr;
	string url;

public:
	WorkInfo(Local<Function> cb, string url);
	~WorkInfo();

	void Start();
	void Run();
};

void AsyncDone(uv_work_t *req, int status) {
	delete static_cast<WorkInfo*>(req->data);
}

void AsyncWork(uv_work_t *req) {
	static_cast<WorkInfo*>(req->data)->Run();
}



WorkInfo::WorkInfo(Local<Function> cb, string url) : strerr(NULL), url(url) {
	this->callback = Persistent<Function>::New(cb);
	this->request.data = this;
}


WorkInfo::~WorkInfo() {
	HandleScope scope;

	if (this->strerr) {
		Local<Value> argv[] = { Exception::TypeError(String::New(this->strerr)) };
	    this->callback->Call(Context::GetCurrent()->Global(), 1, argv);
	} else {
		char *m_title, *m_url, *m_thumb;
		double m_duration;

		quvi_getprop(this->media, QUVIPROP_MEDIATHUMBNAILURL, &m_thumb);
		quvi_getprop(this->media, QUVIPROP_MEDIADURATION, &m_duration);
		quvi_getprop(this->media, QUVIPROP_PAGETITLE, &m_title);
		quvi_getprop(this->media, QUVIPROP_MEDIAURL, &m_url);

		Local<Object> data = Object::New();
		data->Set(String::New("duration"), Number::New(m_duration));
		data->Set(String::New("title"), String::New(m_title));
		data->Set(String::New("thumb"), String::New(m_thumb));
		data->Set(String::New("url"), String::New(m_url));

		quvi_parse_close(&this->media);
		quvi_close(&this->q);

		Local<Value> argv[] = {
			Local<Value>::New(Undefined()),
			Local<Value>::New(data)
		};

	    this->callback->Call(Context::GetCurrent()->Global(), 2, argv);
	}

	this->callback.Dispose();
	scope.Close(Undefined());
}


void WorkInfo::Start() {
	uv_queue_work(
		uv_default_loop(),
		&this->request,
		AsyncWork,
		AsyncDone
	);
}


void WorkInfo::Run() {
	quvi_media_t &m = this->media;
	quvi_t &q = this->q;
	QUVIcode rc;

	rc = quvi_init(&q);
	if (rc != QUVI_OK) {
		quvi_close(&q);

		this->strerr = quvi_strerror(q, rc);
		return;
	}

	quvi_setopt(q, QUVIOPT_NORESOLVE, 1L);
	rc = quvi_parse(q, (char*) this->url.c_str(), &m);
	if (rc != QUVI_OK) {
		quvi_parse_close(&m);
		quvi_close(&q);

		this->strerr = quvi_strerror(q, rc);
		return;
	}
}


Handle<Value> Parse(const Arguments& args) {
	HandleScope scope;

	if (args.Length() != 2) {
		ThrowException(Exception::TypeError(String::New("Wrong number of arguments")));
		return scope.Close(Undefined());
	}

	if (!args[0]->IsString()) {
		ThrowException(Exception::TypeError(String::New("The first argument must be a string")));
		return scope.Close(Undefined());
	}

	if (!args[1]->IsFunction()) {
		ThrowException(Exception::TypeError(String::New("The second argument must be a function")));
		return scope.Close(Undefined());
	}

	String::AsciiValue url(args[0]);
	(new WorkInfo(
		Local<Function>::Cast(args[1]),
		string(*url)
	))->Start();
	
	return scope.Close(Undefined());
}


void Init(Handle<Object> exports) {
	NODE_SET_METHOD(exports, "parse", Parse);
}


NODE_MODULE(quvi, Init)
