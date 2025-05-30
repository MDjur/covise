#ifndef MESSAGE_MACROS_H
#define MESSAGE_MACROS_H

#include "message_types.h"
#include <cassert>
#include <cstring>
#include <memory>
#include <string>
#include <util/tryPrint.h>

namespace covise
{
    class TokenBuffer;
    class MessageSenderInterface;
    class Message;

    namespace detail
    {
        template <typename T>
        bool equals(const T &t1, const T &t2)
        {
            return t1 == t2;
        }
        //for char* nullptr is equal to empty string
        template <>
        inline bool equals<char const *>(const char *const &t1, const char *const &t2)
        {
            if (!t1)
            {
                return !t2 || !strcmp(t2, "");
            }
            if (!t2)
            {
                return !t1 || !strcmp(t1, "");
            }
            return !strcmp(t1, t2);
        }
        struct Dummy
        {
        };

        inline Dummy typeCheck(int msgType, int classType)
        {
            if (msgType != classType)
                throw std::runtime_error("Type mismatch in message_macros: msg type " + std::to_string(msgType) + " , class type " + std::to_string(classType));
            return Dummy{};
        }

        template <class T, class Enable = void>
        struct ValueOrRef
        {
            typedef const T &RetVal;
        }; // primary template

        template <class T>
        struct ValueOrRef<T, typename std::enable_if<std::is_arithmetic<T>::value || std::is_pointer<T>::value>::type>
        {
            typedef T RetVal;
        };

        enum class MessageSyle
        {
            Primary,
            SubMessage
        };

    } //detail

} // namespace covise
//helper macros
//----------------------------------------------------------------------------------------------------------------------------------------------
//EXPAND needed for MSVC
#define EXPAND(x) x

#define STOP_RECURSION()

#define CAT(A, B) A##B
#define SELECT(NAME, NUM) CAT(NAME##_, NUM)

#define GET_COUNT(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, COUNT, ...) COUNT
#define VA_SIZE(...) EXPAND(GET_COUNT(__VA_ARGS__, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 09, 08, 07, 06, 05, 04, 03, 02, 01))

#define VA_SELECT(NAME, ...)                           \
    EXPAND(SELECT(NAME, EXPAND(VA_SIZE(__VA_ARGS__)))) \
    (__VA_ARGS__)

#define MY_OVERLOADED(...) EXPAND(VA_SELECT(MY_OVERLOADED, __VA_ARGS__))

#define MY_OVERLOADED_01(ClassName, MacroName, MacroNameLast, _1) STOP_RECURSION()
#define MY_OVERLOADED_03(ClassName, MacroName, MacroNameLast, _1, _2, _3) STOP_RECURSION()
#define MY_OVERLOADED_05(ClassName, MacroName, MacroNameLast, _1, _2, _3, _4, _5) STOP_RECURSION()
#define MY_OVERLOADED_07(ClassName, MacroName, MacroNameLast, _1, _2, _3, _4, _5, _6, _7) STOP_RECURSION()
#define MY_OVERLOADED_09(ClassName, MacroName, MacroNameLast, _1, _2, _3, _4, _5, _6, _7, _8, _9) STOP_RECURSION()
#define MY_OVERLOADED_11(ClassName, MacroName, MacroNameLast, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11) STOP_RECURSION()
#define MY_OVERLOADED_13(ClassName, MacroName, MacroNameLast, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13) STOP_RECURSION()
#define MY_OVERLOADED_15(ClassName, MacroName, MacroNameLast, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15) STOP_RECURSION()
#define MY_OVERLOADED_17(ClassName, MacroName, MacroNameLast, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17) STOP_RECURSION()
#define MY_OVERLOADED_19(ClassName, MacroName, MacroNameLast, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19) STOP_RECURSION()
#define MY_OVERLOADED_21(ClassName, MacroName, MacroNameLast, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21) STOP_RECURSION()
#define MY_OVERLOADED_23(ClassName, MacroName, MacroNameLast, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23) STOP_RECURSION()
#define MY_OVERLOADED_25(ClassName, MacroName, MacroNameLast, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25) STOP_RECURSION()
#define MY_OVERLOADED_27(ClassName, MacroName, MacroNameLast, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18, _19, _20, _21, _22, _23, _24, _25, _26, _27) STOP_RECURSION()

#define MY_OVERLOADED_02(MacroName, MacroNameLast, T00, N00) MacroNameLast(T00, N00)
#define MY_OVERLOADED_04(MacroName, MacroNameLast, T00, N00, T01, N01) MacroName(T00, N00) MY_OVERLOADED_02(MacroName, MacroNameLast, T01, N01)
#define MY_OVERLOADED_06(MacroName, MacroNameLast, T00, N00, T01, N01, T02, N02) MacroName(T00, N00) MY_OVERLOADED_04(MacroName, MacroNameLast, T01, N01, T02, N02)
#define MY_OVERLOADED_08(MacroName, MacroNameLast, T00, N00, T01, N01, T02, N02, T03, N03) MacroName(T00, N00) MY_OVERLOADED_06(MacroName, MacroNameLast, T01, N01, T02, N02, T03, N03)
#define MY_OVERLOADED_10(MacroName, MacroNameLast, T00, N00, T01, N01, T02, N02, T03, N03, N04, T04) MacroName(T00, N00) MY_OVERLOADED_08(MacroName, MacroNameLast, T01, N01, T02, N02, T03, N03, N04, T04)
#define MY_OVERLOADED_12(MacroName, MacroNameLast, T00, N00, T01, N01, T02, N02, T03, N03, N04, T04, T05, N05) MacroName(T00, N00) MY_OVERLOADED_10(MacroName, MacroNameLast, T01, N01, T02, N02, T03, N03, N04, T04, T05, N05)
#define MY_OVERLOADED_14(MacroName, MacroNameLast, T00, N00, T01, N01, T02, N02, T03, N03, N04, T04, T05, N05, T06, N06) MacroName(T00, N00) MY_OVERLOADED_12(MacroName, MacroNameLast, T01, N01, T02, N02, T03, N03, N04, T04, T05, N05, T06, N06)
#define MY_OVERLOADED_16(MacroName, MacroNameLast, T00, N00, T01, N01, T02, N02, T03, N03, N04, T04, T05, N05, T06, N06, T07, N07) MacroName(T00, N00) MY_OVERLOADED_14(MacroName, MacroNameLast, T01, N01, T02, N02, T03, N03, N04, T04, T05, N05, T06, N06, T07, N07)
#define MY_OVERLOADED_18(MacroName, MacroNameLast, T00, N00, T01, N01, T02, N02, T03, N03, N04, T04, T05, N05, T06, N06, T07, N07, T08, N08) MacroName(T00, N00) MY_OVERLOADED_16(MacroName, MacroNameLast, T01, N01, T02, N02, T03, N03, N04, T04, T05, N05, T06, N06, T07, N07, T08, N08)
#define MY_OVERLOADED_20(MacroName, MacroNameLast, T00, N00, T01, N01, T02, N02, T03, N03, N04, T04, T05, N05, T06, N06, T07, N07, T08, N08, T09, N09) MacroName(T00, N00) MY_OVERLOADED_18(MacroName, MacroNameLast, T01, N01, T02, N02, T03, N03, N04, T04, T05, N05, T06, N06, T07, N07, T08, N08, T09, N09)
#define MY_OVERLOADED_22(MacroName, MacroNameLast, T00, N00, T01, N01, T02, N02, T03, N03, N04, T04, T05, N05, T06, N06, T07, N07, T08, N08, T09, N09, T10, N10) MacroName(T00, N00) MY_OVERLOADED_20(MacroName, MacroNameLast, T01, N01, T02, N02, T03, N03, N04, T04, T05, N05, T06, N06, T07, N07, T08, N08, T09, N09, T10, N10)
#define MY_OVERLOADED_24(MacroName, MacroNameLast, T00, N00, T01, N01, T02, N02, T03, N03, N04, T04, T05, N05, T06, N06, T07, N07, T08, N08, T09, N09, T10, N10, T11, N11) MacroName(T00, N00) MY_OVERLOADED_22(MacroName, MacroNameLast, T01, N01, T02, N02, T03, N03, N04, T04, T05, N05, T06, N06, T07, N07, T08, N08, T09, N09, T10, N10, T11, N11)
#define MY_OVERLOADED_26(MacroName, MacroNameLast, T00, N00, T01, N01, T02, N02, T03, N03, N04, T04, T05, N05, T06, N06, T07, N07, T08, N08, T09, N09, T10, N10, T11, N11, T12, N12) MacroName(T00, N00) MY_OVERLOADED_24(MacroName, MacroNameLast, T01, N01, T02, N02, T03, N03, N04, T04, T05, N05, T06, N06, T07, N07, T08, N08, T09, N09, T10, N10, T11, N11, T12, N12)

#define ACCESSOR(type, name) \
    typename covise::detail::ValueOrRef<type>::RetVal name() const { return CAT(m_, name); }\

#define DECLARATION(type, name) \
    type CAT(m_, name);

#define CONSTRUCTOR_ELEMENT_LAST(type, name) \
    typename covise::detail::ValueOrRef<type>::RetVal name
    
#define CONSTRUCTOR_ELEMENT(type, name) \
    CONSTRUCTOR_ELEMENT_LAST(type, name),

#define CONSTRUCTOR_INITIALIZER_ELEM_LAST(type, name) \
     CAT(m_, name)(name)

#define CONSTRUCTOR_INITIALIZER_ELEM(type, name) \
    CONSTRUCTOR_INITIALIZER_ELEM_LAST(type, name),


#define CONSTRUCTOR_INITIALIZER_TB_LAST(type, name) \
     CAT(m_, name)(covise::detail::get<type>(tb))

#define CONSTRUCTOR_INITIALIZER_TB(type, name) \
    CONSTRUCTOR_INITIALIZER_TB_LAST(type, name),

#define FILL_TOKENBUFFER(type, name) \
    << msg.name()

#define TAKE_FROM_TOKENBUFFER(type, name) \
    >> msg.CAT(m_, name)

#define PRINT_CLASS(type, name)                                          \
    os << #name << ": ";                                                 \
    covise::tryPrintWithError(os, msg.name(), #type, " is not printable"); \
    os << ", ";

#define CHECK_EQUALITY(type, name) \
    covise::detail::equals(c1.name(), c2.name()) &&

//----------------------------------------------------------------------------------------------------------------------------------------------

#define DECL_TB_SERIALIZABLE_STRUCT(ClassName, export, ...)                                \
    struct export ClassName{                                                               \
        EXPAND(MY_OVERLOADED(DECLARATION, DECLARATION, __VA_ARGS__))                       \
            EXPAND(MY_OVERLOADED(ACCESSOR, ACCESSOR, __VA_ARGS__))};                       \
    export covise::TokenBuffer &operator<<(covise::TokenBuffer &tb, const ClassName &msg); \
    export covise::TokenBuffer &operator>>(covise::TokenBuffer &tb, ClassName &msg);       \
    export std::ostream &operator<<(std::ostream &os, const ClassName &msg);

#define IMPL_TB_SERIALIZABLE_STRUCT(ClassName, ...)                                       \
    covise::TokenBuffer &operator<<(covise::TokenBuffer &tb, const ClassName &msg)      \
    {                                                                                        \
        tb EXPAND(MY_OVERLOADED(FILL_TOKENBUFFER, FILL_TOKENBUFFER, __VA_ARGS__));           \
        return tb;                                                                           \
    }                                                                                        \
    covise::TokenBuffer &operator>>(covise::TokenBuffer &tb, ClassName &msg)            \
    {                                                                                        \
        tb EXPAND(MY_OVERLOADED(TAKE_FROM_TOKENBUFFER, TAKE_FROM_TOKENBUFFER, __VA_ARGS__)); \
        return tb;                                                                           \
    }                                                                                        \
    std::ostream &operator<<(std::ostream &os, const ClassName &msg)                    \
    {                                                                                        \
        os << #ClassName << ":" << std::endl;                                                \
        EXPAND(MY_OVERLOADED(PRINT_CLASS, PRINT_CLASS, __VA_ARGS__));                        \
        os << std::endl;                                                                     \
        return os;                                                                           \
    }

//use in header files to declare a type that can be sent and received via messages
//ClassName: message type without COVISE_MESSAGE_
//export: the export macro to use
//...: pairs of typename , value name. Used types must provide stream operators for TokenBuffer
//implements a class with the given members.
//use sendCoviseMessage function to send the message
//receive the message by constructing this class from the message
//also implements operator<< for ostream and tokenbuffer and operator== (for const char* nullptr =="")
#define DECL_MESSAGE_CLASS(ClassName, export, ...)                                                     \
    struct export ClassName                                                                            \
    {                                                                                                  \
        static constexpr covise::detail::MessageSyle Style = covise::detail::MessageSyle::Primary;     \
        ClassName(EXPAND(MY_OVERLOADED(CONSTRUCTOR_ELEMENT, CONSTRUCTOR_ELEMENT_LAST, __VA_ARGS__)));  \
        explicit ClassName(const covise::Message &msg);                                                \
        covise::Message createMessage() const;                                                         \
        EXPAND(MY_OVERLOADED(ACCESSOR, ACCESSOR, __VA_ARGS__))                                         \
                                                                                                       \
    private:                                                                                           \
        EXPAND(MY_OVERLOADED(DECLARATION, DECLARATION, __VA_ARGS__))                                   \
        ClassName(covise::detail::Dummy, covise::TokenBuffer &&tb);                                    \
    };                                                                                                 \
    export covise::TokenBuffer &operator<<(covise::TokenBuffer &tb, const ClassName &msg);             \
    export std::ostream &operator<<(std::ostream &tb, const ClassName &exe);                           \
    export bool sendCoviseMessage(const ClassName &msg, const covise::MessageSenderInterface &sender); \
    export bool operator==(const ClassName &c1, const ClassName &c2);

//use in .cpp with same arguments as DECL_MESSAGE_CLASS exept the export argument
#define IMPL_MESSAGE_CLASS(ClassName, ...)                                                                                                                                   \
    ClassName::ClassName(EXPAND(MY_OVERLOADED(CONSTRUCTOR_ELEMENT, CONSTRUCTOR_ELEMENT_LAST, __VA_ARGS__)))                                                                  \
        : EXPAND(MY_OVERLOADED(CONSTRUCTOR_INITIALIZER_ELEM, CONSTRUCTOR_INITIALIZER_ELEM_LAST, __VA_ARGS__)) {}                                                             \
    ClassName::ClassName(const covise::Message &msg) : ClassName(covise::detail::typeCheck(msg.type, covise::CAT(COVISE_MESSAGE_, ClassName)), covise::TokenBuffer{&msg}) {} \
    ClassName::ClassName(covise::detail::Dummy, covise::TokenBuffer &&tb)                                                                                                    \
        : EXPAND(MY_OVERLOADED(CONSTRUCTOR_INITIALIZER_TB, CONSTRUCTOR_INITIALIZER_TB_LAST, __VA_ARGS__)) {}                                                                 \
    covise::Message ClassName::createMessage() const                                                                                                                         \
    {                                                                                                                                                                        \
        covise::TokenBuffer tb;                                                                                                                                              \
        tb << *this;                                                                                                                                                         \
        covise::Message m{tb};                                                                                                                                               \
        m.type = covise::CAT(COVISE_MESSAGE_, ClassName);                                                                                                                    \
        return m;                                                                                                                                                            \
    }                                                                                                                                                                        \
    covise::TokenBuffer &operator<<(covise::TokenBuffer &tb, const ClassName &msg)                                                                                           \
    {                                                                                                                                                                        \
        tb EXPAND(MY_OVERLOADED(FILL_TOKENBUFFER, FILL_TOKENBUFFER, __VA_ARGS__));                                                                                           \
        return tb;                                                                                                                                                           \
    }                                                                                                                                                                        \
    bool sendCoviseMessage(const ClassName &msg, const covise::MessageSenderInterface &sender)                                                                               \
    {                                                                                                                                                                        \
        auto m = msg.createMessage();                                                                                                                                        \
        return sender.send(&m);                                                                                                                                              \
    }                                                                                                                                                                        \
    std::ostream &operator<<(std::ostream &os, const ClassName &msg)                                                                                                         \
    {                                                                                                                                                                        \
        os << #ClassName << ":" << std::endl;                                                                                                                                \
        EXPAND(MY_OVERLOADED(PRINT_CLASS, PRINT_CLASS, __VA_ARGS__));                                                                                                        \
        os << std::endl;                                                                                                                                                     \
        return os;                                                                                                                                                           \
    }                                                                                                                                                                        \
    bool operator==(const ClassName &c1, const ClassName &c2)                                                                                                                \
    {                                                                                                                                                                        \
        return EXPAND(MY_OVERLOADED(CHECK_EQUALITY, CHECK_EQUALITY, __VA_ARGS__)) true;                                                                                      \
    }

#define DECL_SUB_MESSAGE_CLASS_DETAIL(ClassName, FullClassName, EnumClass, EnumType, export, ...)                  \
    struct export FullClassName : public ClassName                                                                 \
    {                                                                                                              \
        friend struct ClassName;                                                                                   \
        static constexpr covise::detail::MessageSyle Style = covise::detail::MessageSyle::SubMessage;              \
        typedef ClassName Base;                                                                                    \
        explicit FullClassName(EXPAND(MY_OVERLOADED(CONSTRUCTOR_ELEMENT, CONSTRUCTOR_ELEMENT_LAST, __VA_ARGS__))); \
        FullClassName(const FullClassName &) = delete;                                                             \
        FullClassName(FullClassName &&) = default;                                                                 \
        FullClassName &operator=(const FullClassName &) = delete;                                                  \
        FullClassName &operator=(FullClassName &&) = delete;                                                      \
        ~FullClassName() = default;                                                                                \
        covise::Message createMessage() const;                                                                     \
        EXPAND(MY_OVERLOADED(ACCESSOR, ACCESSOR, __VA_ARGS__))                                                     \
                                                                                                                   \
        static const EnumClass subType = EnumClass::EnumType;                                                      \
    private:                                                                                                       \
        EXPAND(MY_OVERLOADED(DECLARATION, DECLARATION, __VA_ARGS__))                                               \
        explicit FullClassName(const covise::Message &msg);                                                        \
        explicit FullClassName(covise::detail::Dummy, covise::TokenBuffer &&tb);                                   \
    };                                                                                                             \
    export covise::TokenBuffer &operator<<(covise::TokenBuffer &tb, const FullClassName &msg);                     \
    export std::ostream &operator<<(std::ostream &tb, const FullClassName &exe);                                   \
    export bool sendCoviseMessage(const FullClassName &msg, const covise::MessageSenderInterface &sender);

#define DECL_SUB_MESSAGE_CLASS(ClassName, EnumClass, EnumType, export, ...) \
    DECL_SUB_MESSAGE_CLASS_DETAIL(ClassName, ClassName##_##EnumType, EnumClass, EnumType, export, __VA_ARGS__)

#define IMPL_SUB_MESSAGE_CLASS_DETAIL(ClassName, FullClassName, EnumClass, EnumType, ...)                                                                                                \
    FullClassName::FullClassName(EXPAND(MY_OVERLOADED(CONSTRUCTOR_ELEMENT, CONSTRUCTOR_ELEMENT_LAST, __VA_ARGS__)))                                                                      \
        : ClassName(EnumClass::EnumType), EXPAND(MY_OVERLOADED(CONSTRUCTOR_INITIALIZER_ELEM, CONSTRUCTOR_INITIALIZER_ELEM_LAST, __VA_ARGS__)) {}                                         \
    FullClassName::FullClassName(const covise::Message &msg) : FullClassName(covise::detail::typeCheck(msg.type, covise::CAT(COVISE_MESSAGE_, ClassName)), covise::TokenBuffer{&msg}) {} \
    FullClassName::FullClassName(covise::detail::Dummy, covise::TokenBuffer &&tb)                                                                                                                               \
        : ClassName(static_cast<EnumClass>(covise::detail::get<int>(tb))), EXPAND(MY_OVERLOADED(CONSTRUCTOR_INITIALIZER_TB, CONSTRUCTOR_INITIALIZER_TB_LAST, __VA_ARGS__)) {}            \
    covise::Message FullClassName::createMessage() const                                                                                                                                 \
    {                                                                                                                                                                                    \
        covise::TokenBuffer tb;                                                                                                                                                          \
        tb << *this;                                                                                                                                                                     \
        covise::Message m{tb};                                                                                                                                                           \
        m.type = covise::CAT(COVISE_MESSAGE_, ClassName);                                                                                                                                \
        return m;                                                                                                                                                                        \
    }                                                                                                                                                                                    \
    covise::TokenBuffer &operator<<(covise::TokenBuffer &tb, const FullClassName &msg)                                                                                                   \
    {                                                                                                                                                                                    \
        tb << static_cast<int>(msg.type) EXPAND(MY_OVERLOADED(FILL_TOKENBUFFER, FILL_TOKENBUFFER, __VA_ARGS__));                                                                         \
        return tb;                                                                                                                                                                       \
    }                                                                                                                                                                                    \
    bool sendCoviseMessage(const FullClassName &msg, const covise::MessageSenderInterface &sender)                                                                                       \
    {                                                                                                                                                                                    \
        auto m = msg.createMessage();                                                                                                                                                    \
        return sender.send(&m);                                                                                                                                                          \
    }                                                                                                                                                                                    \
    std::ostream &operator<<(std::ostream &os, const FullClassName &msg)                                                                                                                 \
    {                                                                                                                                                                                    \
        os << #FullClassName << ":" << std::endl;                                                                                                                                        \
        EXPAND(MY_OVERLOADED(PRINT_CLASS, PRINT_CLASS, __VA_ARGS__));                                                                                                                    \
        os << std::endl;                                                                                                                                                                 \
        return os;                                                                                                                                                                       \
    }

#define IMPL_SUB_MESSAGE_CLASS(ClassName, EnumClass, EnumType, ...) \
    IMPL_SUB_MESSAGE_CLASS_DETAIL(ClassName, ClassName##_##EnumType, EnumClass, EnumType, __VA_ARGS__)

#define DECL_MESSAGE_WITH_SUB_CLASSES(ClassName, EnumClass, export)                                \
    struct export ClassName                                                                        \
    {                                                                                              \
        ClassName(const covise::Message &msg);                                                     \
        virtual ~ClassName() = default;                                                            \
        const EnumClass type;                                                                      \
        static constexpr int MessageType = covise::CAT(COVISE_MESSAGE_, ClassName);                \
        template <typename Derived>                                                                \
        const Derived &unpackOrCast() const                                                        \
        {                                                                                          \
            covise::detail::typeCheck(static_cast<int>(Derived::subType), static_cast<int>(type)); \
            if (!m_msg)                                                                            \
            {                                                                                      \
                return *dynamic_cast<const Derived *>(this);                                       \
            }                                                                                      \
            if (!m_subMsg)                                                                         \
            {                                                                                      \
                m_subMsg.reset(new Derived(*m_msg));                                               \
            }                                                                                      \
            return *dynamic_cast<const Derived *>(m_subMsg.get());                                 \
        }                                                                                          \
        template <typename Derived>                                                                \
        Derived createDerived() const                                                              \
        {                                                                                          \
            assert(m_msg);                                                                         \
            return Derived{*m_msg};                                                                \
        }                                                                                          \
                                                                                                   \
    protected:                                                                                     \
        ClassName(EnumClass);                                                                      \
                                                                                                   \
    private:                                                                                       \
        ClassName(covise::detail::Dummy, covise::TokenBuffer &&tb, const covise::Message &msg);    \
        const covise::Message *m_msg = nullptr;                                                    \
        mutable std::shared_ptr<ClassName> m_subMsg;                                               \
    };

#define IMPL_MESSAGE_WITH_SUB_CLASSES(ClassName, EnumClass)                                                                                                                       \
    ClassName::ClassName(const covise::Message &msg) : ClassName(covise::detail::typeCheck(msg.type, covise::CAT(COVISE_MESSAGE_, ClassName)), covise::TokenBuffer(&msg), msg) {} \
    ClassName::ClassName(covise::detail::Dummy, covise::TokenBuffer &&tb, const covise::Message &msg)                                                                             \
        : type(static_cast<EnumClass>(covise::detail::get<int>(tb))), m_msg(&msg) {}                                                                                              \
    ClassName::ClassName(EnumClass t)                                                                                                                                             \
        : type(t)                                                                                                                                                                 \
    {                                                                                                                                                                             \
    }

#endif // !  MESSAGE_MACROS_H
