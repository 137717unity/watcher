#include <string>
#include <utility>

#include "directory_record.h"
#include "../message.h"
#include "../message_buffer.h"
#include "polled_root.h"

using std::string;
using std::move;

PolledRoot::PolledRoot(string &&root_path, ChannelID channel_id) :
  root(new DirectoryRecord(move(root_path))),
  channel_id{channel_id},
  iterator(root)
{
  //
}

size_t PolledRoot::advance(MessageBuffer &buffer, size_t throttle_allocation)
{
  ChannelMessageBuffer channel_buffer(buffer, channel_id);
  BoundPollingIterator bound_iterator(iterator, channel_buffer);

  return bound_iterator.advance(throttle_allocation);
}
