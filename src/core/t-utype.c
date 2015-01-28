#include <stdio.h>
/***********************************************************************
**
**  REBOL [R3] Language Interpreter and Run-time Environment
**
**  Copyright 2012 REBOL Technologies
**  REBOL is a trademark of REBOL Technologies
**
**  Licensed under the Apache License, Version 2.0 (the "License");
**  you may not use this file except in compliance with the License.
**  You may obtain a copy of the License at
**
**  http://www.apache.org/licenses/LICENSE-2.0
**
**  Unless required by applicable law or agreed to in writing, software
**  distributed under the License is distributed on an "AS IS" BASIS,
**  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
**  See the License for the specific language governing permissions and
**  limitations under the License.
**
************************************************************************
**
**  Module:  t-utype.c
**  Summary: user defined datatype
**  Section: datatypes
**  Author:  Carl Sassenrath
**  Notes:   Implemented by giuliolunati@gmail.com
**
***********************************************************************/

#include "sys-core.h"
#include "reb-utype.h"

#define	SET_UTYPE(v,f) VAL_OBJ_FRAME(v) = (f), VAL_SET(v, REB_UTYPE)

static REBOOL Same_Object(REBVAL *val, REBVAL *arg);

/***********************************************************************
**
*/	REBINT CT_Utype(REBVAL *a, REBVAL *b, REBINT mode)
/*
***********************************************************************/
{
	REBVAL *f, m;
	SET_INTEGER(&m, mode);
	if (mode == 3) {
		if (VAL_TYPE(a) == VAL_TYPE(b)
			&& VAL_OBJ_FRAME(a) == VAL_OBJ_FRAME(b)
		) return TRUE; else return FALSE;
	}
	if (IS_UTYPE(a)) {
		f = GET_UTYPE_METHOD(".COMPARE",a);
		if (f && IS_FUNCTION(f)) {
			f = Apply_Func(0,f,a,b,&m,0);
			if (IS_TRUE(f)) return TRUE;
		}
	}
	if (IS_UTYPE(b)) {
		f = GET_UTYPE_METHOD(".COMPARE",b);
		if (f && IS_FUNCTION(f)) {
			f = Apply_Func(0,f,a,b,&m,0);
			if (IS_TRUE(f)) return TRUE;
		}
	}
	return FALSE;
}


/***********************************************************************
**
*/	REBFLG MT_Utype(REBVAL *out, REBVAL *data, REBCNT type)
/*
***********************************************************************/
{
	return FALSE;
}


/***********************************************************************
**
*/	REBTYPE(Utype)
/*
***********************************************************************/
{
	REBVAL *value = D_ARG(1);
	REBVAL *arg = D_ARG(2);
	REBINT i, n, sym;
	REBVAL *val;
	REBSER *obj, *src_obj;
	REBCNT type = 0;

	switch (action) {
	case A_MAKE:
		// make utype! [init]|object
		if ((IS_DATATYPE(value)) 
				&& (REB_UTYPE == VAL_DATATYPE(value))
		) {
			if (IS_BLOCK(arg)) {
				obj = Make_Object(0, VAL_BLK_DATA(arg));
				SET_UTYPE(ds, obj); // GC save
				arg = Do_Bind_Block(obj, arg); // GC-OK
				goto ok; // returns obj
			} else if (IS_OBJECT(arg)) {
				obj = VAL_OBJ_FRAME(arg);
				goto ok; // returns obj
			} else Trap_Make(REB_UTYPE, arg);
		}
		// make parent ....
		if (IS_UTYPE(value) || IS_OBJECT(value)) {
			src_obj  = VAL_OBJ_FRAME(value);
			// make parent none | []
			if (IS_NONE(arg) || (IS_BLOCK(arg) && IS_EMPTY(arg))) {
				obj = Copy_Block_Values(src_obj, 0, SERIES_TAIL(src_obj), TS_CLONE);
				Rebind_Frame(src_obj, obj);
				goto ok; // returns obj
			}
			// make parent [...]
			if (IS_BLOCK(arg)) {
				obj = Make_Object(src_obj, VAL_BLK_DATA(arg));
				Rebind_Frame(src_obj, obj);
				SET_UTYPE(ds, obj);
				arg = Do_Bind_Block(obj, arg); // GC-OK
				goto ok; // returns obj
			}
			// make parent object|utype
			if (IS_UTYPE(arg) || IS_OBJECT(arg)) {
				obj = Merge_Frames(src_obj, VAL_OBJ_FRAME(arg));
				goto ok; // returns obj
			}
		}
		Trap_Make(VAL_TYPE(value), value);
	case A_COPY: {
		// Note: words are not copied and bindings not changed!
		REBU64 types = 0;
		if (D_REF(ARG_COPY_PART)) Trap0(RE_BAD_REFINES);
		if (D_REF(ARG_COPY_DEEP)) {
			types |= CP_DEEP | (D_REF(ARG_COPY_TYPES) ? 0 : TS_STD_SERIES);
		}
		if D_REF(ARG_COPY_TYPES) {
			arg = D_ARG(ARG_COPY_KINDS);
			if (IS_DATATYPE(arg)) types |= TYPESET(VAL_DATATYPE(arg));
			else types |= VAL_TYPESET(arg);
		}
		VAL_OBJ_FRAME(value) = obj = Copy_Block(VAL_OBJ_FRAME(value), 0);
		if (types != 0) Copy_Deep_Values(obj, 1, SERIES_TAIL(obj), types);
		goto ok; // returns obj
	}
	case A_REFLECT:
		action = What_Reflector(arg); // zero on error
		if (action == OF_SPEC) {
			if (!VAL_MOD_SPEC(value)) return R_NONE;
			VAL_OBJ_FRAME(value) = VAL_MOD_SPEC(value);
			VAL_SET(value, REB_OBJECT);
			goto ok; // returns obj
		}
		// Adjust for compatibility with PICK:
		if (action == OF_VALUES) action = 2;
		else if (action == OF_BODY) action = 3;
		if (action < 1 || action > 3) Trap_Reflect(VAL_TYPE(value), arg);
		if (THROWN(value)) Trap0(RE_THROW_USAGE);
		Set_Block(value, Make_Object_Block(VAL_OBJ_FRAME(value), action));
		goto ok; // returns obj
	}

	// for binary action, try also 2nd ARG if 1st fails
	n = (
			(IS_BINARY_ACT(action) || action == A_TO) ?
			2 : 1
	);
	for(i=1; i<=n ; i++) {
		val = D_ARG(i);
		if (! IS_UTYPE(val)) continue;
		switch (action) {
			case A_ABSOLUTE: sym = Make_Word(".ABSOLUTE",0); break;
			case A_ADD: sym = Make_Word(".ADD",0); break;
			case A_AND: sym = Make_Word(".AND",0); break;
			case A_APPEND: sym = Make_Word(".APPEND",0); break;
			case A_AT: sym = Make_Word(".AT",0); break;
			case A_BACK: sym = Make_Word(".BACK",0); break;
			case A_CHANGE: sym = Make_Word(".CHANGE",0); break;
			case A_CLEAR: sym = Make_Word(".CLEAR",0); break;
			case A_CLOSE: sym = Make_Word(".CLOSE",0); break;
			case A_COMPLEMENT: sym = Make_Word(".COMPLEMENT",0); break;
			case A_CREATE: sym = Make_Word(".CREATE",0); break;
			case A_DELETE: sym = Make_Word(".DELETE",0); break;
			case A_DIVIDE: sym = Make_Word(".DIVIDE",0); break;
			case A_EVENQ: sym = Make_Word(".EVEN?",0); break;
			case A_FIND: sym = Make_Word(".FIND",0); break;
			case A_HEAD: sym = Make_Word(".HEAD",0); break;
			case A_HEADQ: sym = Make_Word(".HEAD?",0); break;
			case A_INDEXQ: sym = Make_Word(".INDEX?",0); break;
			case A_INSERT: sym = Make_Word(".INSERT",0); break;
			case A_LENGTHQ: sym = Make_Word(".LENGTHQ",0); break;
			case A_MODIFY: sym = Make_Word(".MODIFY",0); break;
			case A_MULTIPLY: sym = Make_Word(".MULTIPLY",0); break;
			case A_NEGATE: sym = Make_Word(".NEGATE",0); break;
			case A_NEXT: sym = Make_Word(".NEXT",0); break;
			case A_ODDQ: sym = Make_Word(".ODD?",0); break;
			case A_OPEN: sym = Make_Word(".OPEN",0); break;
			case A_OPENQ: sym = Make_Word(".OPEN?",0); break;
			case A_OR: sym = Make_Word(".OR",0); break;
			case A_PASTQ: sym = Make_Word(".PAST?",0); break;
			case A_POKE: sym = Make_Word(".POKE",0); break;
			case A_POWER: sym = Make_Word(".POWER",0); break;
			case A_QUERY: sym = Make_Word(".QUERY",0); break;
			case A_RANDOM: sym = Make_Word(".RANDOM",0); break;
			case A_READ: sym = Make_Word(".READ",0); break;
			case A_REMAINDER: sym = Make_Word(".REMAINDER",0); break;
			case A_REMOVE: sym = Make_Word(".REMOVE",0); break;
			case A_RENAME: sym = Make_Word(".RENAME",0); break;
			case A_REVERSE: sym = Make_Word(".REVERSE",0); break;
			case A_ROUND: sym = Make_Word(".ROUND",0); break;
			case A_SELECT: sym = Make_Word(".SELECT",0); break;
			case A_SKIP: sym = Make_Word(".SKIP",0); break;
			case A_SORT: sym = Make_Word(".SORT",0); break;
			case A_SUBTRACT: sym = Make_Word(".SUBTRACT",0); break;
			case A_SWAP: sym = Make_Word(".SWAP",0); break;
			case A_TAIL: sym = Make_Word(".TAIL",0); break;
			case A_TAILQ: sym = Make_Word(".TAILQ",0); break;
			case A_TAKE: sym = Make_Word(".TAKE",0); break;
			case A_TO: sym = Make_Word(".TO",0); break;
			case A_UPDATE: sym = Make_Word(".UPDATE",0); break;
			case A_WRITE: sym = Make_Word(".WRITE",0); break;
			case A_XOR: sym = Make_Word(".XOR",0); break;

			default: Trap_Action(REB_UTYPE, action);
		} // switch

		val = Find_Word_Value(VAL_OBJ_FRAME(val), sym);
		if (val && IS_FUNCTION(val)) {
			val = Apply_Func(0,val,D_ARG(1),D_ARG(2),D_ARG(3),D_ARG(4),D_ARG(5),D_ARG(7),D_ARG(7),D_ARG(8),D_ARG(9),0);
			// fail if unset!
			if ((! IS_DATATYPE(val)) 
					|| (REB_UNSET != VAL_DATATYPE(val))) {
				DS_RET_VALUE(val);
				return R_RET;
			}
			// failed
		}
	} // while
	Trap_Action(REB_UTYPE, action);

ok:
	VAL_SET(value, REB_UTYPE);
	VAL_OBJ_FRAME(value) = obj;

	DS_RET_VALUE(value);
	return R_RET;
}

/***********************************************************************
**
*/	REBVAL *Try_Utype_Method(REBVAL *ds, REBYTE *word)
/*
***********************************************************************/
{
	REBVAL *f;
	if (!D_ARG(1) || ! IS_UTYPE(D_ARG(1))) return NULL;
	f = GET_UTYPE_METHOD(word,D_ARG(1));
	if (!f || !IS_FUNCTION(f)) Trap_Arg(D_ARG(1));
	f =  Apply_Func(0,f,D_ARG(1),D_ARG(2),D_ARG(3),D_ARG(4),D_ARG(5),D_ARG(7),D_ARG(7),D_ARG(8),D_ARG(9),0);
	DS_RET_VALUE(f);
	return f;
}

