/*
 *
 *
 * Distributed under the OpenDDS License.
 * See: http://www.opendds.org/license.html
 */

#include "typeobject_generator.h"
#include "be_extern.h"

#include "utl_identifier.h"

#include "topic_keys.h"

using std::string;
using namespace AstTypeClassification;

bool
typeobject_generator::gen_enum(AST_Enum*, UTL_ScopedName* name,
  const std::vector<AST_EnumVal*>& contents, const char*)
{
  be_global->add_include("dds/DCPS/TypeObject.h", BE_GlobalData::STREAM_H);
  NamespaceGuard ng;
  const string clazz = scoped(name);
  const string decl = "getTypeIdentifier<" + clazz + ">";
  Function gti(decl.c_str(), "RcHandle<XTypes::TypeIdentifier>", "");
  gti.endArgs();
  be_global->impl_ << "  return RcHandle<XTypes::TypeIdentifier>();\n"; //TODO
  return true;
}

namespace {

void
call_get_type_identifier(AST_Type* type)
{
  const Classification fld_cls = classify(type);
  const string suffix = (fld_cls & CL_ARRAY) ? "_tag" : "";
  be_global->impl_ << "              "
    "getTypeIdentifier<" << scoped(type->name()) << suffix << ">()";
}

}

bool
typeobject_generator::gen_struct(AST_Structure* node, UTL_ScopedName* name,
  const std::vector<AST_Field*>& fields, AST_Type::SIZE_TYPE, const char*)
{
  be_global->add_include("dds/DCPS/TypeObject.h", BE_GlobalData::STREAM_H);
  NamespaceGuard ng;
  const string clazz = scoped(name);

  {
    const string decl_gto = "getTypeObject<" + clazz + ">";
    Function gto(decl_gto.c_str(), "const XTypes::TypeObject&", "");
    gto.endArgs();

    // TODO: Pick the appropriate flags.
    // TODO: Support struct inheritance.
    be_global->impl_ <<
      "  static const XTypes::TypeObject to = XTypes::TypeObject::make(\n"
      "    XTypes::MinimalTypeObject::make(\n"
      "      XTypes::MinimalStructType::make(XTypes::IS_FINAL | XTypes::IS_NESTED | XTypes::IS_AUTOID_HASH,\n"
      "        XTypes::MinimalStructHeader::make(\n"
      "          getTypeIdentifier<void>(),\n"
      "          XTypes::MinimalTypeDetail::make()\n"
      "        ),\n"
      "        XTypes::MinimalStructMemberSeq()\n";

    ACE_CDR::ULong member_id = 0;
    for (std::vector<AST_Field*>::const_iterator pos = fields.begin(), limit = fields.end(); pos != limit; ++pos) {
      be_global->impl_ <<
        "        .append(\n"
        "          XTypes::MinimalStructMember::make(\n"
        "            XTypes::CommonStructMember::make(\n"
        "              " << member_id++ << ",\n"
      // TODO: Set StructMemberFlags.
        "              0,\n";
      call_get_type_identifier((*pos)->field_type());
      be_global->impl_ << "\n"
        "            ),\n"
        "            XTypes::MinimalMemberDetail::make(\"" << (*pos)->local_name()->get_string() << "\")\n"
        "          )\n"
        "        )\n";
    }

  be_global->impl_ <<
    "      )\n"
    "    )\n"
    "  );\n"
    "  return to;\n";
  }
  {
    const string decl_gti = "getTypeIdentifier<" + clazz + ">";
    Function gti(decl_gti.c_str(), "RcHandle<XTypes::TypeIdentifier>", "");
    gti.endArgs();
    be_global->impl_ <<
      "  static const RcHandle<XTypes::TypeIdentifier> ti = XTypes::makeTypeIdentifier(getTypeObject<" << clazz << ">());\n"
      "  return ti;\n";
  }
  return true;
}

bool
typeobject_generator::gen_typedef(AST_Typedef*, UTL_ScopedName* name,
  AST_Type* type, const char*)
{
  string suffix;
  switch (type->node_type()) {
  case AST_Decl::NT_array:
    suffix = "_tag";
    // fall-through
  case AST_Decl::NT_sequence:
    break;
  default:
    return true;
  }

  be_global->add_include("dds/DCPS/TypeObject.h",
                         BE_GlobalData::STREAM_H);
  NamespaceGuard ng;
  const string clazz = scoped(name);
  const string decl = "getTypeIdentifier<" + clazz + suffix + ">";
  Function gti(decl.c_str(), "RcHandle<XTypes::TypeIdentifier>", "");
  gti.endArgs();
  be_global->impl_ << "  return RcHandle<XTypes::TypeIdentifier>();\n"; //TODO
  return true;
}

bool
typeobject_generator::gen_union(AST_Union* node, UTL_ScopedName* name,
  const std::vector<AST_UnionBranch*>& branches, AST_Type* discriminator,
  const char*)
{
  return true;
}
