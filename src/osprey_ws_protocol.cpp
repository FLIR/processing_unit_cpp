#include "osprey_ws_protocol.hpp"
#include "msgpack.hpp"

#include <memory>
#include <string>
#include <iostream>
#include <sstream>
#include <cassert>
//#include <jsonconfig.hpp> // json
#include <memory>

using json = nlohmann::json;

namespace ProcessingUnit
{


const std::string Pre("Map{");
const std::string Post("}");

const std::string MessageTypeLabel("messageType");
const std::string JobIdLabel("jobId");
const std::string InfoLabel("info");
const std::string PayloadLabel("payload");
const std::string IsReadyLabel("isReady");
const std::string DescriptionLabel("description");
const std::string MetadataLabel("metadata");

const std::string StartMessageType("start");
const std::string ReadyMessageType("ready");
const std::string DataMessageType("data");
const std::string ContinueMessageType("continue");
const std::string EndMessageType("end");
const std::string UnknownMessageType("unknown");

Message::MessageType MessageTypeFromString(const std::string &MsgTypeStr)
{
    if (MsgTypeStr == StartMessageType)
    {
        return Message::Start;
    }
    else if (MsgTypeStr == ReadyMessageType)
    {
        return Message::Ready;
    }
    else if (MsgTypeStr == DataMessageType)
    {
        return Message::Data;
    }
    else if (MsgTypeStr == ContinueMessageType)
    {
        return Message::Continue;
    }
    else if (MsgTypeStr == EndMessageType)
    {
        return Message::End;
    }
    else
    {
        return Message::Unknown;
    }
}

std::string MessageTypeToString(Message::MessageType MsgType)
{
    switch (MsgType)
    {
    case Message::Start:
        return StartMessageType;
        break;
    case Message::Ready:
        return ReadyMessageType;
        break;
    case Message::Data:
        return DataMessageType;
        break;
    case Message::Continue:
        return ContinueMessageType;
        break;
    case Message::End:
        return EndMessageType;
        break;
    case Message::Unknown:
    default:
        return UnknownMessageType;
    }
}

Message::MessageType SafeMessageTypeExtractFromJson(nlohmann::json &Json)
{
    Message::MessageType RetType = Message::Unknown;
    if (Json.find(MessageTypeLabel) != Json.end())
    {
        const std::string MsgTypeStr = Json[MessageTypeLabel];
        return MessageTypeFromString(MsgTypeStr);
    }
    return RetType;
}

/////////////////////////////////////////////////////////////

Message::Message() : m_MessageType(Message::Unknown) {}
Message::Message(const std::string &MsgTypeString) { SetMessageType(MsgTypeString); }
Message::Message(MessageType MsgType) : m_MessageType(MsgType) {}
Message::MessageType Message::GetMessageType() const { return m_MessageType; }
std::string Message::GetMessageTypeAsString() const { return MessageTypeToString(m_MessageType); }
std::string Message::GetInfoLabel() { return InfoLabel; }
std::string Message::GetPayloadLabel() { return PayloadLabel; }
std::string Message::GetPreamble() { return Pre; }
std::string Message::GetPostamble() { return Post; }
void Message::SetMessageType(const std::string &MessageType) { m_MessageType = MessageTypeFromString(MessageType); }

/////////////////////////////////////////////////////////////

StartMessage::StartMessage() : Message(StartMessageType), m_JobId(), m_InfoJson() {}
StartMessage::StartMessage(const std::string &JobId, const json &Info) : Message(StartMessageType), m_JobId(JobId), m_InfoJson(Info) {}
std::string StartMessage::GetJobId() const { return m_JobId; }
json StartMessage::GetInfoJson() const { return m_InfoJson; }

void StartMessage::SetJobId(const std::string &JobId) { m_JobId = JobId; }
//void StartMessage::SetInfo(const std::string& Info) { m_Info = Info; }
void StartMessage::SetInfoJson(const json &Info) { m_InfoJson = Info; }

void to_json(json &J, const StartMessage &M)
{
    J = json{{MessageTypeLabel, M.GetMessageTypeAsString()}, {JobIdLabel, M.GetJobId()}, {InfoLabel, M.GetInfoJson()}};
}
void from_json(const json &J, StartMessage &M)
{
    M.SetMessageType(J.at(MessageTypeLabel).get<std::string>());
    M.SetJobId(J.at(JobIdLabel).get<std::string>());
    //M.SetInfo(J.at(InfoLabel).get<std::string>());
    M.SetInfoJson(J.at(InfoLabel));
}

/////////////////////////////////////////////////////////////

DataMessage::DataMessage() : Message(DataMessageType), m_Metadata(), m_Payload() {}
DataMessage::DataMessage(const std::string &Metadata, const std::vector<char> &Payload) : Message(DataMessageType), m_Metadata(Metadata), m_Payload(Payload) {}
DataMessage::DataMessage(const std::string &Metadata, const std::vector<unsigned char> &Payload) : Message(DataMessageType), m_Metadata(Metadata), m_Payload(Payload.begin(), Payload.end()) {}
std::string DataMessage::GetMetaData() const { return m_Metadata; }
std::vector<char> DataMessage::GetPayloadData() const { return m_Payload; }
std::size_t DataMessage::GetPayloadSize() const { return m_Payload.size(); }

void DataMessage::SetMetadata(const std::string &Metadata) { m_Metadata = Metadata; }

void DataMessage::SetPayload(const std::vector<char> &Payload)
{
    m_Payload = Payload;
}

void to_json(json &J, const DataMessage &M)
{
    J = json{{MessageTypeLabel, M.GetMessageTypeAsString()}, {MetadataLabel, M.GetMetaData()}};
}

void from_json(const json &J, DataMessage &M)
{
    M.SetMessageType(J.at(MessageTypeLabel).get<std::string>());
    if (J.count(MetadataLabel) > 0)
    {
        M.SetMetadata(J.at(MetadataLabel).get<std::string>());
    }
}

/////////////////////////////////////////////////////////////

ReadyMessage::ReadyMessage() : Message(ReadyMessageType), m_IsReady(false), m_Description("") {}
ReadyMessage::ReadyMessage(bool IsReady, const std::string &Description) : Message(ReadyMessageType), m_IsReady(IsReady), m_Description(Description) {}
bool ReadyMessage::IsReady() const { return m_IsReady; }
std::string ReadyMessage::GetDescription() const { return m_Description; }

void ReadyMessage::SetIsReady(bool IsReady) { m_IsReady = IsReady; }
void ReadyMessage::SetDescription(const std::string &Description) { m_Description = Description; }

void to_json(json &J, const ReadyMessage &M)
{
    if (M.IsReady())
    {
        J = json{{MessageTypeLabel, M.GetMessageTypeAsString()}, {IsReadyLabel, M.IsReady()}};
    }
    else
    {
        J = json{{MessageTypeLabel, M.GetMessageTypeAsString()}, {IsReadyLabel, M.IsReady()}, {DescriptionLabel, M.GetDescription()}};
    }
}

void from_json(const json &J, ReadyMessage &M)
{
    M.SetIsReady(J.at(IsReadyLabel).get<bool>());
    //M.SetDescription(J.at(DescriptionLabel).get<std::string>());
    if (J.count(DescriptionLabel) > 0)
    {
        M.SetDescription(J.at(DescriptionLabel).get<std::string>());
    }
}

/////////////////////////////////////////////////////////////

ContinueMessage::ContinueMessage() : Message(ContinueMessageType) {}

void to_json(json &J, const ContinueMessage &M)
{
    J = json{{MessageTypeLabel, M.GetMessageTypeAsString()}};
}
void from_json(const json &J, ContinueMessage &M)
{
    M.SetMessageType(J.at(MessageTypeLabel).get<std::string>());
}

/////////////////////////////////////////////////////////////

EndMessage::EndMessage() : Message(EndMessageType) {}

void to_json(json &J, const EndMessage &M)
{
    J = json{{MessageTypeLabel, M.GetMessageTypeAsString()}};
}
void from_json(const json &J, EndMessage &M)
{
    M.SetMessageType(J.at(MessageTypeLabel).get<std::string>());
}

/////////////////////////////////////////////////////////////

Message::MessageType MessageReader::GetMessageType() const
{
    return m_CurrMessageType;
}

std::unique_ptr<StartMessage> MessageReader::GetStartMessage() const
{
    std::unique_ptr<StartMessage> Msg(new StartMessage());
    if (m_CurrMessageType == Message::Start)
    {
        *Msg = m_Json;
    }
    return Msg;
}

std::unique_ptr<ReadyMessage> MessageReader::GetReadyMessage() const
{
    std::unique_ptr<ReadyMessage> Msg(new ReadyMessage());
    if (m_CurrMessageType == Message::Ready)
    {
        *Msg = m_Json;
    }
    return Msg;
}

std::unique_ptr<EndMessage> MessageReader::GetEndMessage() const
{
    std::unique_ptr<EndMessage> Msg(new EndMessage());
    if (m_CurrMessageType == Message::End)
    {
        *Msg = m_Json;
    }
    return Msg;
}

std::unique_ptr<ContinueMessage> MessageReader::GetContinueMessage() const
{
    std::unique_ptr<ContinueMessage> Msg(new ContinueMessage());
    if (m_CurrMessageType == Message::Continue)
    {
        *Msg = m_Json;
    }
    return Msg;
}

std::unique_ptr<DataMessage> MessageReader::GetDataMessage()
{
    std::unique_ptr<DataMessage> Msg(new DataMessage());
    if (m_CurrMessageType == Message::Data)
    {
        *Msg = m_Json;
        Msg->SetPayload(m_Payload);
    }
    return Msg;
}

std::unique_ptr<Message> MessageReader::GetMessage() const
{
    switch (m_CurrMessageType)
    {
    case Message::Start:
    {
        std::unique_ptr<StartMessage> Msg(new StartMessage());
        *Msg = m_Json;
        return Msg;
    }
    break;
    case Message::Ready:
    {
        //return ReadyMessageType;
        std::unique_ptr<ReadyMessage> Msg(new ReadyMessage());
        *Msg = m_Json;
        return Msg;
    }
    break;
    case Message::Data:
    {
        std::unique_ptr<DataMessage> Msg(new DataMessage());
        *Msg = m_Json;
        Msg->SetPayload(m_Payload);
        return Msg;
    }
    break;
    case Message::Continue:
    {
        std::unique_ptr<ContinueMessage> Msg(new ContinueMessage());
        *Msg = m_Json;
        return Msg;
    }
    break;
    case Message::End:
    {
        std::unique_ptr<EndMessage> Msg(new EndMessage());
        *Msg = m_Json;
        return Msg;
    }
    break;
    case Message::Unknown:
    default:
        std::unique_ptr<Message> Msg(new Message());
        return Msg;
    }
}

bool MessageReader::Parse(const std::string &Input)
{
    bool RetVal = false;

    if (Input.size())
    {

        msgpack::unpacked UnpackedData = msgpack::unpack(Input.data(), Input.size());
        typedef std::map<std::string, msgpack::object> MapType;

        MapType FullMessage;

        try
        {
            FullMessage = UnpackedData.get().as<MapType>();
        }
        catch (std::exception &Ex)
        {
            std::cerr << "Error: We couldn't convert the incoming message to a map, wrong protocol? " << std::endl;
            std::cerr << "\tError message: " << Ex.what() << std::endl;
        }

        for (MapType::iterator Iter = FullMessage.begin(); Iter != FullMessage.end(); ++Iter)
        {
            if (Iter->first == InfoLabel)
            {
                std::string JsonString;
                Iter->second.convert<std::string>(JsonString);
                m_Json = nlohmann::json::parse(JsonString);
                m_CurrMessageType = SafeMessageTypeExtractFromJson(m_Json);
                RetVal = true;
            }
            else if (Iter->first == PayloadLabel)
            {
                Iter->second.convert<std::vector<char>>(m_Payload);
            }
            else
            {
                std::cerr << "Error: Unsupported message received, did protocol change? "
                          << "Key: " << Iter->first << "Val: " << Iter->second << std::endl;
            }
        }
    }
    return RetVal;
}
} // namespace ProcessingUnit
