/*
 * The MIT License
 *
 * Copyright (c) 2011 GitHub, Inc
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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#ifndef __H_RUGGED_BINDINGS__
#define __H_RUGGED_BINDINGS__

// tell rbx not to use it's caching compat layer
// by doing this we're making a promize to RBX that
// we'll never modify the pointers we get back from RSTRING_PTR
#define RSTRING_NOT_MODIFIED

#include <ruby.h>

#include <assert.h>
#include <git2.h>
#include <git2/odb_backend.h>


/**
 *	The following methods from the libgit2 API
 *	are not wrapped because they are redundant or
 *	too low level:
 *
 *	__attribute__
 *	git_blob_close
 *	git_blob_lookup
 *	git_blob_lookup_prefix
 *	git_commit_close
 *	git_commit_id
 *	git_commit_lookup_prefix
 *	git_oid_allocfmt
 *	git_oid_cpy
 *	git_oid_ncmp
 *	git_oid_pathfmt
 *	git_oid_to_string
 *	git_reference_listall
 *	git_strarray_free
 *	git_tree_id
 *	git_tree_lookup
 *	git_tree_lookup_prefix
 *	git_tree_close
 *	git_tag_id
 *	git_tag_lookup
 *	git_tag_lookup_prefix
 *	git_tag_close
 *	git_object__size
 *
 *	git_tag_target_oid
 *	git_odb_write
 *	git_odb_add_alternate
 *	git_odb_backend_loose
 *	git_odb_backend_pack
 *	git_odb_close
 *	git_odb_new
 *	git_odb_open_rstream
 *	git_odb_read_header
 *	git_odb_read_prefix
 *	git_index_entry_stage
 *	git_commit_create_v
 *	git_commit_parent_oid
 *	git_commit_time_offset
 *	git_commit_tree_oid
 */

/*
 * Initialization functions 
 */
void Init_rugged_object();
void Init_rugged_commit();
void Init_rugged_tree();
void Init_rugged_tag();
void Init_rugged_blob();
void Init_rugged_index();
void Init_rugged_repo();
void Init_rugged_revwalk();
void Init_rugged_signature();
void Init_rugged_reference();
void Init_rugged_config();

VALUE rb_git_object_init(git_otype type, int argc, VALUE *argv, VALUE self);

VALUE rugged_raw_read(git_repository *repo, const git_oid *oid);

VALUE rugged_signature_new(const git_signature *sig);
VALUE rugged_object_new(VALUE repository, git_object *object);
VALUE rugged_index_new(VALUE owner, git_index *index);
VALUE rugged_config_new(git_config *cfg);

git_otype rugged_get_otype(VALUE rb_type);

git_signature *rugged_signature_get(VALUE rb_person);
git_object *rugged_object_get(git_repository *repo, VALUE object_value, git_otype type);

typedef struct {
	git_odb_backend parent;
	VALUE self;
} rugged_backend;

typedef struct {
	git_object	*object;
	VALUE owner;
} rugged_object;

typedef struct {
	git_repository *repo;
	VALUE backends;
} rugged_repository;

typedef struct {
	git_index *index;
	VALUE owner;
} rugged_index;

typedef struct {
	git_revwalk *walk;
	VALUE owner;
} rugged_walker;

typedef struct {
	git_reference *ref;
	VALUE owner;
} rugged_reference;

#define CSTR2SYM(s) (ID2SYM(rb_intern((s))))

#define RUGGED_OBJ_UNWRAP(_rb, _type, _c) {\
	rugged_object *_rugged_obj; \
	Data_Get_Struct(_rb, rugged_object, _rugged_obj); \
	_c = (_type *)(_rugged_obj->object); \
}

#define RUGGED_OBJ_OWNER(_rb, _val) {\
	rugged_object *_rugged_obj;\
	Data_Get_Struct(_rb, rugged_object, _rugged_obj);\
	_val = (_rugged_obj->owner);\
}

static inline void rugged_exception_check(int errorcode)
{
	if (errorcode < 0)
		rb_raise(rb_eRuntimeError, "%s\n(error code %d)", git_lasterror(), errorcode);

	git_clearerror();
}

static inline int rugged_parse_bool(VALUE boolean)
{
	if (TYPE(boolean) != T_TRUE && TYPE(boolean) != T_FALSE)
		rb_raise(rb_eTypeError, "Expected boolean value");

	return boolean ? 1 : 0;
}

/* support for string encodings in 1.9 */
#ifdef HAVE_RUBY_ENCODING_H

#	include <ruby/encoding.h>

static inline VALUE rugged_str_new(const char *str, long len, rb_encoding *rb_enc)
{
	VALUE encoded_string;
	rb_encoding *internal_encoding = NULL;

	internal_encoding = rb_default_internal_encoding();

	if (rb_enc) {
		encoded_string = rb_enc_str_new(str, len, rb_enc);
	} else {
		encoded_string = rb_str_new(str, len);
	}

	if (internal_encoding) {
		encoded_string = rb_str_export_to_enc(encoded_string, internal_encoding);
	}

	return encoded_string;
}


static inline VALUE rugged_str_new2(const char *str, rb_encoding *rb_enc)
{
	VALUE encoded_string;
	rb_encoding *internal_encoding = NULL;

	internal_encoding = rb_default_internal_encoding();
	encoded_string = rb_str_new2(str);

	if (rb_enc) {
		rb_enc_associate(encoded_string, rb_enc);
	}

	if (internal_encoding) {
		encoded_string = rb_str_export_to_enc(encoded_string, internal_encoding);
	}

	return encoded_string;
}

#	define rugged_str_ascii(ptr, len) rb_enc_str_new((ptr), (len), rb_ascii8bit_encoding())
#else
#	define rugged_str_new(str, len, rb_enc)  rb_str_new(str, len)
#	define rugged_str_new2(str, rb_enc) rb_str_new2(str)
#	define rugged_str_ascii(ptr, len) rb_str_new(ptr, len)
#endif

static inline VALUE rugged_create_oid(const git_oid *oid)
{
	char out[40];
	git_oid_fmt(out, oid);
	return rugged_str_new(out, 40, NULL);
}

#endif
