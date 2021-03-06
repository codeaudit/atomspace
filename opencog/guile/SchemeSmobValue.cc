/*
 * SchemeSmobValue.c
 *
 * Scheme small objects (SMOBS) for ProtoAtoms.
 *
 * Copyright (c) 2008,2009,2016 Linas Vepstas <linas@linas.org>
 */

#include <cstddef>
#include <libguile.h>

#include <opencog/atoms/base/FloatValue.h>
#include <opencog/atoms/base/LinkValue.h>
#include <opencog/atoms/base/StringValue.h>

#include <opencog/guile/SchemeSmob.h>

using namespace opencog;

/* ============================================================== */
/** Return true if s is a value */

SCM SchemeSmob::ss_value_p (SCM s)
{
	if (not SCM_SMOB_PREDICATE(SchemeSmob::cog_misc_tag, s))
		return SCM_BOOL_F;

	scm_t_bits misctype = SCM_SMOB_FLAGS(s);
	if (COG_PROTOM == misctype)
		return SCM_BOOL_T;

	return SCM_BOOL_F;
}

/* ============================================================== */
/**
 * Convert argument into a list of floats.
 */
std::vector<double>
SchemeSmob::verify_float_list (SCM svalue_list, const char * subrname, int pos)
{
	// Verify that second arg is an actual list. Allow null list
	// (which is rather unusual, but legit.  Allow embedded nulls
	// as this can be convenient for writing scheme code.
	if (!scm_is_pair(svalue_list) and !scm_is_null(svalue_list))
		scm_wrong_type_arg_msg(subrname, pos, svalue_list, "a list of float-pt values");
	return scm_to_float_list(svalue_list);
}

std::vector<double>
SchemeSmob::scm_to_float_list (SCM svalue_list)
{
	std::vector<double> valist;
	SCM sl = svalue_list;
	while (scm_is_pair(sl)) {
		SCM svalue = SCM_CAR(sl);

		if (not scm_is_null(svalue)) {
			double v = scm_to_double(svalue);
			valist.emplace_back(v);
		}
		sl = SCM_CDR(sl);
	}
	return valist;
}

/**
 * Convert argument into a list of protoatoms.
 */
std::vector<ProtoAtomPtr>
SchemeSmob::verify_protom_list (SCM svalue_list, const char * subrname, int pos)
{
	// Verify that second arg is an actual list. Allow null list
	// (which is rather unusual, but legit.  Allow embedded nulls
	// as this can be convenient for writing scheme code.
	if (!scm_is_pair(svalue_list) and !scm_is_null(svalue_list))
		scm_wrong_type_arg_msg(subrname, pos, svalue_list, "a list of protoatom values");
	return scm_to_protom_list(svalue_list);
}

std::vector<ProtoAtomPtr>
SchemeSmob::scm_to_protom_list (SCM svalue_list)
{
	std::vector<ProtoAtomPtr> valist;
	SCM sl = svalue_list;
	while (scm_is_pair(sl)) {
		SCM svalue = SCM_CAR(sl);

		if (not scm_is_null(svalue)) {
			ProtoAtomPtr pa(scm_to_protom(svalue));
			valist.emplace_back(pa);
		}
		sl = SCM_CDR(sl);
	}
	return valist;
}

/**
 * Convert argument into a list of strings.
 */
std::vector<std::string>
SchemeSmob::verify_string_list (SCM svalue_list, const char * subrname, int pos)
{
	// Verify that second arg is an actual list. Allow null list
	// (which is rather unusual, but legit).  Allow embedded nulls,
	// as this can be convenient for writing scheme code.
	if (!scm_is_pair(svalue_list) and !scm_is_null(svalue_list))
		scm_wrong_type_arg_msg(subrname, pos, svalue_list, "a list of string values");

	return scm_to_string_list(svalue_list);
}

std::vector<std::string>
SchemeSmob::scm_to_string_list (SCM svalue_list)
{
	std::vector<std::string> valist;
	SCM sl = svalue_list;
	while (scm_is_pair(sl)) {
		SCM svalue = SCM_CAR(sl);

		if (not scm_is_null(svalue)) {
			char * v = scm_to_utf8_string(svalue);
			valist.emplace_back(v);
		}
		sl = SCM_CDR(sl);
	}
	return valist;
}

/* ============================================================== */
/**
 * Create a new value, of named type stype, and value vector svect
 */
SCM SchemeSmob::ss_new_value (SCM stype, SCM svalue_list)
{
	Type t = verify_atom_type(stype, "cog-new-value", 1);

	ProtoAtomPtr pa;
	if (FLOAT_VALUE == t)
	{
		std::vector<double> valist;
		valist = verify_float_list(svalue_list, "cog-new-value", 2);
		pa = createFloatValue(valist);
	}

	else if (LINK_VALUE == t)
	{
		std::vector<ProtoAtomPtr> valist;
		valist = verify_protom_list(svalue_list, "cog-new-value", 2);
		pa = createLinkValue(valist);
	}

	else if (STRING_VALUE == t)
	{
		std::vector<std::string> valist;
		valist = verify_string_list(svalue_list, "cog-new-value", 2);
		pa = createStringValue(valist);
	}

	scm_remember_upto_here_1(svalue_list);
	return protom_to_scm(pa);
}

/* ============================================================== */

SCM SchemeSmob::ss_set_value (SCM satom, SCM skey, SCM svalue)
{
	Handle atom(verify_handle(satom, "cog-set-value!", 1));
	Handle key(verify_handle(skey, "cog-set-value!", 2));

	// If svalue is actually a value, just use it.
	// If it is a list, assume its a list of values.
	ProtoAtomPtr pa;
	if (scm_is_pair(svalue)) {
		SCM sitem = SCM_CAR(svalue);

		if (scm_is_number(sitem))
		{
			std::vector<double> fl = scm_to_float_list(svalue);
			pa = createFloatValue(fl);
		}
		else if (scm_is_string(sitem))
		{
			std::vector<std::string> fl = scm_to_string_list(svalue);
			pa = createStringValue(fl);
		}
		else if (scm_is_symbol(sitem))
		{
			// The code below allows the following to be evaluated:
			// (define x 0.44) (define y 0.55)
			// (cog-set-value! (Concept "foo") (Predicate "bar") '(x y))
			// Here, x and y are symbols, the symbol lookup gives
			// variables, and the variable deref gives 0.44, 0.55.
			SCM sl = svalue;
			SCM newl = SCM_EOL;
			while (scm_is_pair(sl)) {
				SCM sym = SCM_CAR(sl);
				if (scm_is_symbol(sym))
					newl = scm_cons(scm_variable_ref(scm_lookup(sym)), newl);
				else if (scm_is_true(scm_variable_p(sym)))
					newl = scm_cons(scm_variable_ref(sym), newl);
				else
					newl = scm_cons(sym, newl);
				sl = SCM_CDR(sl);
			}
			newl = scm_reverse(newl);
			return ss_set_value(satom, skey, newl);
		}
		else if (scm_is_true(scm_list_p(svalue)))
		{
			verify_protom(sitem, "cog-set-value!", 3);
			std::vector<ProtoAtomPtr> fl = scm_to_protom_list(svalue);
			pa = createLinkValue(fl);
		}
		else
		{
			scm_wrong_type_arg_msg("cog-set-value!", 3, svalue,
				"a list of protoatom values");
		}
	}
	else
	{
		pa = verify_protom(svalue, "cog-set-value!", 3);
	}

	atom->setValue(key, pa);
	return satom;
}

SCM SchemeSmob::ss_value (SCM satom, SCM skey)
{
	Handle atom(verify_handle(satom, "cog-value", 1));
	Handle key(verify_handle(skey, "cog-value", 2));

	try
	{
		return protom_to_scm(atom->getValue(key));
	}
	catch (const std::exception& ex)
	{
		throw_exception(ex, "cog-value", scm_cons(satom, skey));
	}
	return SCM_EOL;
}

/* ===================== END OF FILE ============================ */
