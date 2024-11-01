// model_resources.h

namespace ExtensionsManager {
namespace Resources {

namespace PluginFiles {
extern const char* const ConfigFileName;
extern const char* const InterfaceFileName;
}  // namespace PluginFiles

namespace ConfigFields {
namespace Root {
extern const char* const Id;
extern const char* const Version;
extern const char* const Meta;
extern const char* const Configuration;
extern const char* const Interface;
extern const char* const Gateway;
extern const char* const Type;

}  // namespace Root

namespace Meta {
extern const char* const Placement;
extern const char* const Text;
extern const char* const Name;
extern const char* const Description;
extern const char* const Author;
extern const char* const Prefix;
extern const char* const Size;
extern const char* const Type;
extern const char* const Language;
extern const char* const Label;
}  // namespace Meta

namespace Types {
extern const char* const Boolean;
extern const char* const String;
extern const char* const Int;
extern const char* const Float;
}  // namespace Types

namespace Values {
extern const char* const Node;
extern const char* const None;
extern const char* const Language;
extern const char* const Static;
extern const char* const Custom;
}  // namespace Values

namespace Configuration {
extern const char* const Type;
extern const char* const Inputs;
extern const char* const Script;
extern const char* const Name;
extern const char* const Description;
extern const char* const Field;
extern const char* const Required;
extern const char* const Type;
extern const char* const Format;
extern const char* const ScriptId;
}  // namespace Configuration

namespace DbFields {
extern const char* const Plugin;
extern const char* const Version;

extern const char* const MetaName;
extern const char* const MetaAuthor;
extern const char* const MetaDescription;
extern const char* const MetaPrefix;
extern const char* const MetaSize;
extern const char* const MetaType;
extern const char* const MetaLanguage;
extern const char* const ShortName;
extern const char* const Placement;
}  // namespace DbFields

}  // namespace ConfigFields
}  // namespace Resources
}  // namespace ExtensionsManager
