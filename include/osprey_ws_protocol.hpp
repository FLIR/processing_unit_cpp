#ifndef _OSPREY_WS_PROTOCOL_H_
#define _OSPREY_WS_PROTOCOL_H_

#include <msgpack.hpp>
#include <string>

#include "json/jsonconfig.hpp"

/*!
    This file holds different classes that optionally can be split over multiple files.
    It has as main goal to bundle classes / methods around the Osprey Websocket Protocol.

    Flow typical connection:
    Start (Prism -> processor)
    Ready (Processor -> prism)
    Data (Prism -> Processor)
    Continue (Processor -> Processor)
    Data (Processor -> Processor)
    Continue (Prism -> Processor) // this tells the processor it's ok to send end or more data
    End (Prism -> Processor)
    End (Processor -> Prism)
    // Continue is for both sides: any side receiving data should respond with continue

    We're not throwing errors everywhere, we simply made sure nothing crashes when wrong data is given.
    If exception handling is preferred, feel free to amend.

    One can easily create a message by for example
    StartMessage Msg(JobId, JsonString);

    This message can be automatically parsed to and from json and msgpack.

    * If we for example want access to JSON, we can easily do
    json JsonMessage = Msg;
    // We prefer one to use the getters and setters however

    * If we for example want to create a message based on Json, we can do this easily:
    json Something = ....
    EndMessage Msg = Something;

    * If we wish to pack it and send it one can easily do
    msgpack::pack(Stream, Msg);
    // Where Stream could be whatever you want it to be.

    * If we wish to unpack it, we can easily do the following
    MessageReader Reader;
    const std::string Input = Stream.str(); // could be whatever data stream, that we can conver to string for now.
    bool CouldParse = Reader.Parse(Input);
    // Above returns false when it couldn't be parsed properly and true when it could

    Message::MessageType Type = Reader.GetMessageType();
    switch(Type)
    {
        case Message::Start:
            StartMessage Msg = Reader.GetStartMessage();
            const std::string JobId = Msg.GetJobId();
            const std::string Info = Msg.GetInfo();
            break;
        case Message::Ready:
            ReadyMessage Msg = Reader.GetReadyMessage();
            ...
            break;
        case Message::Data:
            DataMessage Msg = Reader.GetDataMessage();
            ...
            break;
        case Message::Continue:
            ContinueMessage Msg = Reader.GetContinueMessage();
            ...
            break;
        case Message::End;
            EndMessage Msg=  Reader.GetEndMessage();
            break;
        default: break;
    }
*/

/*!
    Base type for the different messages we support
*/

namespace ProcessingUnit
{
class Message
{
public:
    enum MessageType
    {
        Start,
        Ready,
        Data,
        Continue,
        End,
        Unknown
    };

    Message();
    Message(const std::string &MsgTypeStr);
    Message(MessageType MsgType);
    virtual ~Message() {}

    virtual MessageType GetMessageType() const final;
    virtual std::string GetMessageTypeAsString() const final;
    static std::string GetPreamble();
    static std::string GetPostamble();
    static std::string GetInfoLabel();
    static std::string GetPayloadLabel();

    bool IsStartMessage() const;
    bool IsDataMessage() const;
    bool IsReadyMessage() const;
    bool IsEndMessage() const;

    void SetMessageType(const std::string &MessageType);

private:
    //std::string m_MessageType;
    MessageType m_MessageType;
};

class StartMessage : public Message
{
public:
    StartMessage();
    StartMessage(const std::string &JobId, const json &Info);
    std::string GetJobId() const;
    json GetInfoJson() const; //<! todo not sure if this is good 'nough

    void SetJobId(const std::string &JobId);
    void SetInfoJson(const json &Info);

private:
    std::string m_JobId;
    json m_InfoJson;
};
void to_json(nlohmann::json &J, const StartMessage &M);
void from_json(const nlohmann::json &J, StartMessage &M);

/////////////////////////////////////////////////////////////

class DataMessage : public Message
{
public:
    DataMessage();
    DataMessage(const std::string &Metadata, const std::vector<char> &Payload);
    DataMessage(const std::string &Metadata, const std::vector<unsigned char> &Payload);
    std::string GetMetaData() const;
    std::vector<char> GetPayloadData() const;
    std::size_t GetPayloadSize() const;

    void SetMetadata(const std::string &Metadata);
    void SetPayload(const std::vector<char> &Payload);

private:
    std::string m_Metadata;
    std::vector<char> m_Payload;
};
void to_json(nlohmann::json &J, const DataMessage &M);
void from_json(const nlohmann::json &J, DataMessage &M);

/////////////////////////////////////////////////////////////

class ReadyMessage : public Message
{
public:
    ReadyMessage();
    ReadyMessage(bool IsReady, const std::string &Description = "");
    bool IsReady() const;
    std::string GetDescription() const;

    void SetIsReady(bool IsReady);
    void SetDescription(const std::string &Description);

private:
    bool m_IsReady;
    std::string m_Description;
};
void to_json(nlohmann::json &J, const ReadyMessage &M);
void from_json(const nlohmann::json &J, ReadyMessage &M);

/////////////////////////////////////////////////////////////

class ContinueMessage : public Message
{
public:
    ContinueMessage();

private:
};
void to_json(nlohmann::json &J, const ContinueMessage &M);
void from_json(const nlohmann::json &J, ContinueMessage &M);

/////////////////////////////////////////////////////////////

class EndMessage : public Message
{
public:
    EndMessage();

private:
};
void to_json(nlohmann::json &J, const EndMessage &M);
void from_json(const nlohmann::json &J, EndMessage &M);

/////////////////////////////////////////////////////////////

class MessageReader
{
public:
    /*!
        Parses an osprey ws message in the msgpack format.
        Depending on the message type a StartMessage, EndMessage or DataMessage can be requested.
        So correct flow is: Parse, Check Message type using GetMessageType(),
        depending on that type ask the proper getter like GetStartMessage or GetEndMessage or GetDataMessage
    */
    bool Parse(const std::string &Input);

    Message::MessageType GetMessageType() const;
    std::unique_ptr<StartMessage> GetStartMessage() const;
    std::unique_ptr<EndMessage> GetEndMessage() const;
    std::unique_ptr<ReadyMessage> GetReadyMessage() const;
    std::unique_ptr<DataMessage> GetDataMessage();
    std::unique_ptr<ContinueMessage> GetContinueMessage() const;

    std::unique_ptr<Message> GetMessage() const;

private:
    Message::MessageType m_CurrMessageType;
    nlohmann::json m_Json;
    std::vector<char> m_Payload;
};
} // namespace ProcessingUnit

/////////////////////////////////////////////////////////////
namespace msgpack
{
MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
{
    namespace adaptor
    {

    template <>
    struct pack<ProcessingUnit::StartMessage>
    {
        template <typename Stream>
        packer<Stream> &operator()(msgpack::packer<Stream> &O, ProcessingUnit::StartMessage const &Msg) const
        {
            json JsonMessage = Msg;
            O.pack_map(1);
            O.pack(ProcessingUnit::Message::GetInfoLabel());
            O.pack(JsonMessage.dump(0));

            return O;
        }
    };

    template <>
    struct pack<ProcessingUnit::ReadyMessage>
    {
        template <typename Stream>
        packer<Stream> &operator()(msgpack::packer<Stream> &O, ProcessingUnit::ReadyMessage const &Msg) const
        {
            json JsonMessage = Msg;
            O.pack_map(1);
            O.pack(ProcessingUnit::Message::GetInfoLabel());
            O.pack(JsonMessage.dump(0));

            return O;
        }
    };

    template <>
    struct pack<ProcessingUnit::DataMessage>
    {
        template <typename Stream>
        packer<Stream> &operator()(msgpack::packer<Stream> &O, ProcessingUnit::DataMessage const &Msg) const
        {
            json JsonMessage = Msg;
            O.pack_map(2);
            O.pack(ProcessingUnit::Message::GetInfoLabel());
            O.pack(JsonMessage.dump(0));
            O.pack(ProcessingUnit::Message::GetPayloadLabel());
            O.pack(Msg.GetPayloadData());

            return O;
        }
    };

    template <>
    struct pack<ProcessingUnit::ContinueMessage>
    {
        template <typename Stream>
        packer<Stream> &operator()(msgpack::packer<Stream> &O, ProcessingUnit::ContinueMessage const &Msg) const
        {
            json JsonMessage = Msg;
            O.pack_map(1);
            O.pack(ProcessingUnit::Message::GetInfoLabel());
            O.pack(JsonMessage.dump(0));

            return O;
        }
    };

    template <>
    struct pack<ProcessingUnit::EndMessage>
    {
        template <typename Stream>
        packer<Stream> &operator()(msgpack::packer<Stream> &O, ProcessingUnit::EndMessage const &Msg) const
        {
            json JsonMessage = Msg;
            O.pack_map(1);
            O.pack(ProcessingUnit::Message::GetInfoLabel());
            O.pack(JsonMessage.dump(0));

            return O;
        }
    };

    } // namespace adaptor
} // MSGPACK_API_VERSION_NAMESPACE(MSGPACK_DEFAULT_API_NS)
} // namespace msgpack

#endif // _OSPREY_WS_PROTOCOL_H_
