/*
 * Copyright (c) 2012 Christoph Schulz
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "tesseract.h"
#include "image.h"
#include "util.h"
#include <sstream>
#include <strngs.h>
#include <resultiterator.h>

using namespace v8;
using namespace node;

void Tesseract::Init(Handle<Object> target)
{
    Local<FunctionTemplate> constructor_template = FunctionTemplate::New(New);
    constructor_template->SetClassName(String::NewSymbol("Tesseract"));
    constructor_template->InstanceTemplate()->SetInternalFieldCount(1);
    Local<ObjectTemplate> proto = constructor_template->PrototypeTemplate();
    proto->SetAccessor(String::NewSymbol("image"), GetImage, SetImage);
    proto->SetAccessor(String::NewSymbol("rectangle"), GetRectangle, SetRectangle);
    proto->SetAccessor(String::NewSymbol("pageSegMode"), GetPageSegMode, SetPageSegMode);
    proto->Set(String::NewSymbol("clear"),
               FunctionTemplate::New(Clear)->GetFunction());
    proto->Set(String::NewSymbol("clearAdaptiveClassifier"),
               FunctionTemplate::New(ClearAdaptiveClassifier)->GetFunction());
    proto->Set(String::NewSymbol("thresholdImage"),
               FunctionTemplate::New(ThresholdImage)->GetFunction());
    proto->Set(String::NewSymbol("findRegions"),
               FunctionTemplate::New(FindRegions)->GetFunction());
    proto->Set(String::NewSymbol("findTextLines"),
               FunctionTemplate::New(FindTextLines)->GetFunction());
    proto->Set(String::NewSymbol("findWords"),
               FunctionTemplate::New(FindWords)->GetFunction());
    proto->Set(String::NewSymbol("findSymbols"),
               FunctionTemplate::New(FindSymbols)->GetFunction());
    proto->Set(String::NewSymbol("findText"),
               FunctionTemplate::New(FindText)->GetFunction());
    target->Set(String::NewSymbol("Tesseract"),
                Persistent<Function>::New(constructor_template->GetFunction()));
}

Handle<Value> Tesseract::New(const Arguments &args)
{
    HandleScope scope;
    Local<String> datapath;
    Local<String> lang;
    Local<Object> image;
    if (args.Length() == 1 && args[0]->IsString()) {
        datapath = args[0]->ToString();
        lang = String::New("eng");
    } else if (args.Length() == 2 && args[0]->IsString() && args[1]->IsString()) {
        datapath = args[0]->ToString();
        lang = args[1]->ToString();
    } else if (args.Length() == 3 && args[0]->IsString() && args[1]->IsString()
               && Image::HasInstance(args[2])) {
        datapath = args[0]->ToString();
        lang = args[1]->ToString();
        image = args[2]->ToObject();
    } else {
        return THROW(TypeError, "cannot convert argument list to "
                     "(datapath: String) or "
                     "(datapath: String, language: String) or "
                     "(datapath: String, language: String, image: Image)");
    }
    Tesseract* obj = new Tesseract(*String::AsciiValue(datapath),
                                   *String::AsciiValue(lang));
    if (!image.IsEmpty()) {
        obj->image_ = Persistent<Object>::New(image->ToObject());
        obj->api_.SetImage(Image::Pixels(obj->image_));
    }
    obj->Wrap(args.This());
    return args.This();
}

Handle<Value> Tesseract::GetImage(Local<String> prop, const AccessorInfo &info)
{
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    return obj->image_;
}

void Tesseract::SetImage(Local<String> prop, Local<Value> value, const AccessorInfo &info)
{
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    if (Image::HasInstance(value)) {
        if (!obj->image_.IsEmpty()) {
            obj->image_.Dispose();
            obj->image_.Clear();
        }
        obj->image_ = Persistent<Object>::New(value->ToObject());
        obj->api_.SetImage(Image::Pixels(obj->image_));
    } else {
        THROW(TypeError, "value must be of type Image");
    }
}

Handle<Value> Tesseract::GetRectangle(Local<String> prop, const AccessorInfo &info)
{
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    return obj->rectangle_;
}

void Tesseract::SetRectangle(Local<String> prop, Local<Value> value, const AccessorInfo &info)
{
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    Local<Object> rect = value->ToObject();
    Handle<String> x = String::NewSymbol("x");
    Handle<String> y = String::NewSymbol("y");
    Handle<String> width = String::NewSymbol("width");
    Handle<String> height = String::NewSymbol("height");
    if (value->IsObject() && rect->Has(x) && rect->Has(y) &&
            rect->Has(width) && rect->Has(height)) {
        if (!obj->rectangle_.IsEmpty()) {
            obj->rectangle_.Dispose();
            obj->rectangle_.Clear();
        }
        obj->rectangle_ = Persistent<Object>::New(rect);
        obj->api_.SetRectangle(rect->Get(x)->Int32Value(), rect->Get(y)->Int32Value(),
                               rect->Get(width)->Int32Value(), rect->Get(height)->Int32Value());
    } else {
        THROW(TypeError, "value must be of type Object with at least "
              "x, y, width and height properties");
    }
}

Handle<Value> Tesseract::GetPageSegMode(Local<String> prop, const AccessorInfo &info)
{
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    switch (obj->api_.GetPageSegMode()) {
    case tesseract::PSM_OSD_ONLY:
        return String::New("osd_only");
    case tesseract::PSM_AUTO_OSD:
        return String::New("auto_osd");
    case tesseract::PSM_AUTO_ONLY:
        return String::New("auto_only");
    case tesseract::PSM_AUTO:
        return String::New("auto");
    case tesseract::PSM_SINGLE_COLUMN:
        return String::New("single_column");
    case tesseract::PSM_SINGLE_BLOCK_VERT_TEXT:
        return String::New("single_block_vert_text");
    case tesseract::PSM_SINGLE_BLOCK:
        return String::New("single_block");
    case tesseract::PSM_SINGLE_LINE:
        return String::New("single_line");
    case tesseract::PSM_SINGLE_WORD:
        return String::New("single_word");
    case tesseract::PSM_CIRCLE_WORD:
        return String::New("circle_word");
    case tesseract::PSM_SINGLE_CHAR:
        return String::New("single_char");
    default:
        return THROW(Error, "cannot convert internal PSM to String");
    }
}

void Tesseract::SetPageSegMode(Local<String> prop, Local<Value> value, const AccessorInfo &info)
{
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(info.This());
    String::AsciiValue pageSegMode(value);
    if (strcmp("osd_only", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_OSD_ONLY);
    } else if (strcmp("auto_osd", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_AUTO_OSD);
    } else if (strcmp("auto_only", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_AUTO_ONLY);
    } else if (strcmp("auto", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_AUTO);
    } else if (strcmp("single_column", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_SINGLE_COLUMN);
    } else if (strcmp("single_block_vert_text", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_SINGLE_BLOCK_VERT_TEXT);
    } else if (strcmp("single_block", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
    } else if (strcmp("single_line", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_SINGLE_LINE);
    } else if (strcmp("single_word", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_SINGLE_WORD);
    } else if (strcmp("circle_word", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_CIRCLE_WORD);
    } else if (strcmp("single_char", *pageSegMode) == 0) {
        obj->api_.SetPageSegMode(tesseract::PSM_SINGLE_CHAR);
    } else {
        THROW(TypeError, "value must be of type String. "
              "Valid values are: "
              "osd_only, auto_osd, auto_only, auto, single_column, "
              "single_block_vert_text, single_block, single_line, "
              "single_word, circle_word, single_char");
    }
}

Handle<Value> Tesseract::Clear(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    obj->api_.Clear();
    return args.This();
}

Handle<Value> Tesseract::ClearAdaptiveClassifier(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    obj->api_.ClearAdaptiveClassifier();
    return args.This();
}

Handle<Value> Tesseract::ThresholdImage(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    Pix *pix = obj->api_.GetThresholdedImage();
    if (pix) {
        return scope.Close(Image::New(pix));
    } else {
        return scope.Close(Null());
    }
}

Handle<Value> Tesseract::FindRegions(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    Local<Object> boxes = Array::New();
    Boxa* boxa = obj->api_.GetRegions(NULL);
    if (boxa) {
        for (int i = 0; i < boxa->n; ++i) {
            boxes->Set(i, createBox(boxa->box[i]));
        }
        boxaDestroy(&boxa);
    }
    return scope.Close(boxes);
}

Handle<Value> Tesseract::FindTextLines(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    Local<Object> boxes = Array::New();
    Boxa* boxa = obj->api_.GetTextlines(NULL, NULL);
    if (boxa) {
        for (int i = 0; i < boxa->n; ++i) {
            boxes->Set(i, createBox(boxa->box[i]));
        }
        boxaDestroy(&boxa);
    }
    return scope.Close(boxes);
}

Handle<Value> Tesseract::FindWords(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    Local<Object> boxes = Array::New();
    Boxa* boxa = obj->api_.GetWords(NULL);
    if (boxa) {
        for (int i = 0; i < boxa->n; ++i) {
            boxes->Set(i, createBox(boxa->box[i]));
        }
        boxaDestroy(&boxa);
    }
    return scope.Close(boxes);
}

Handle<Value> Tesseract::FindSymbols(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    if (obj->api_.Recognize(NULL) != 0) {
        return THROW(Error, "Internal tesseract error");
    }
    tesseract::ResultIterator* resultIter = obj->api_.GetIterator();
    Local<Array> symbols = Array::New();
    int symbolIndex = 0;
    if (resultIter && !resultIter->Empty(tesseract::RIL_WORD)) {
        do {
            tesseract::ChoiceIterator choiceIter = tesseract::ChoiceIterator(*resultIter);
            Local<Array> choices = Array::New();
            int choiceIndex = 0;
            do {
                const char* text = choiceIter.GetUTF8Text();
                if (!text) {
                    break;
                }
                // Transform choice to object.
                Local<Object> choice = Object::New();
                choice->Set(String::NewSymbol("text"),
                            String::New(text));
                choice->Set(String::NewSymbol("confidence"),
                            Number::New(choiceIter.Confidence()));
                // Append choice to choices list.
                choices->Set(choiceIndex, choice);
                ++choiceIndex;
            } while (choiceIter.Next());
            // Append choices to symbols list.
            if (choices->Length() > 0) {
                symbols->Set(symbolIndex, choices);
                ++symbolIndex;
            }
        } while (resultIter->Next(tesseract::RIL_SYMBOL));
        // Cleanup.
        delete resultIter;
    }
    return scope.Close(symbols);
}

Handle<Value> Tesseract::FindText(const Arguments &args)
{
    HandleScope scope;
    Tesseract* obj = ObjectWrap::Unwrap<Tesseract>(args.This());
    if (args.Length() >= 1 && args[0]->IsString()) {
        String::AsciiValue mode(args[0]);
        const char *text = NULL;
        if (strcmp("plain", *mode) == 0) {
            text = obj->api_.GetUTF8Text();
        } else if (strcmp("unlv", *mode) == 0) {
            text = obj->api_.GetUNLVText();
        } else if (strcmp("hocr", *mode) == 0 && args.Length() == 2 && args[1]->IsInt32()) {
            text = obj->api_.GetHOCRText(args[1]->Int32Value());
        } else if (strcmp("box", *mode) == 0 && args.Length() == 2 && args[1]->IsInt32()) {
            text = obj->api_.GetBoxText(args[1]->Int32Value());
        }
        if (text) {
            Local<String> textString = String::New(text);
            delete[] text;
            return scope.Close(textString);
        }
        return THROW(Error, "Internal tesseract error");
    }
    return THROW(TypeError, "cannot convert argument list to "
                 "(\"plain\") or "
                 "(\"unlv\") or "
                 "(\"hocr\", pageNumber: Int32) or "
                 "(\"box\", pageNumber: Int32)");
}

Tesseract::Tesseract(const char *datapath, const char *language)
{
    int res = api_.Init(datapath, language, tesseract::OEM_DEFAULT);
    api_.SetVariable("save_blob_choices", "T");
    assert(res == 0);
}

Tesseract::~Tesseract()
{
    api_.End();
}
