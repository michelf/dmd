
import std.c.stdio;

alias void* id; // untyped Objective-C object pointer

extern (Objective-C)
interface Class {
	NSObject alloc(); // implicit selector
}

extern (Objective-C)
interface NSObject {
	NSObject initWithUTF8String(in char *str); // implicit selector
	void release() [release];
}

extern (C) void NSLog(NSObject, ...);
extern (C) Class objc_lookUpClass(const char* name);
extern (C) id objc_msgSend(id obj, const char* sel);

void main() {
	auto c = objc_lookUpClass("NSString");
	auto o = c.alloc().initWithUTF8String("hello");
	NSLog(o);
	o.release();
}