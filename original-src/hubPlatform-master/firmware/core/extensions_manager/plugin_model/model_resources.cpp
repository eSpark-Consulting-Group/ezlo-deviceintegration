#include "extensions_manager/plugin_model/model_resources.h"

namespace ExtensionsManager {
namespace Resources {

namespace PluginFiles {
const char* const ConfigFileName = "config.json";
const char* const InterfaceFileName = "interface.json";
}  // namespace PluginFiles

namespace ConfigFields {

namespace Root {
const char* const Id = "id";
const char* const Version = "version";
const char* const Meta = "meta";
const char* const Configuration = "configuration";
const char* const Interface = "interface";
const char* const Gateway = "gateway";
const char* const Type = "type";

}  // namespace Root

namespace Meta {
const char* const Author = "author";
const char* const Placement = "placement";
const char* const Text = "text";
const char* const Name = "name";
const char* const Description = "description";
const char* const Prefix = "prefix";
const char* const Size = "size";
const char* const Type = "type";
const char* const Language = "language";
const char* const Label = "label";
}  // namespace Meta

namespace Types {
const char* const Boolean = "boolean";
const char* const String = "string";
const char* const Int = "int";
const char* const Float = "float";
}  // namespace Types

namespace Values {
const char* const Node = "node";
const char* const None = "none";
const char* const Language = "language";
const char* const Static = "static";
const char* const Custom = "custom";
}  // namespace Values

namespace Configuration {
const char* const Type = "type";
const char* const Inputs = "inputs";
const char* const Script = "script";
const char* const Name = "name";
const char* const Description = "description";
const char* const Field = "field";
const char* const Required = "required";
const char* const Format = "format";
const char* const ScriptId = "scriptId";
}  // namespace Configuration

namespace DbFields {
const char* const Plugin = "plugin";
const char* const Version = "version";
const char* const ShortName = "shortName";

const char* const MetaName = "metaName";
const char* const MetaAuthor = "metaAuthor";
const char* const MetaPrefix = "metaPrefix";
const char* const MetaDescription = "metaDescription";
const char* const MetaSize = "metaSize";
const char* const MetaType = "metaType";
const char* const MetaLanguage = "metaLanguage";
const char* const Placement = "placement";
}  // namespace DbFields

}  // namespace ConfigFields
}  // namespace Resources
}  // namespace ExtensionsManager
