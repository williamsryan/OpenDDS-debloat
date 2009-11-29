// -*- C++ -*-
//
// $Id$
#include "Writer.h"
#include "common.h"
#include "TestException.h"
#include "tests/DCPS/FooType5/FooDefTypeSupportC.h"
#include "tests/DCPS/common/TestSupport.h"
#include "ace/Atomic_Op_T.h"
#include "ace/OS_NS_unistd.h"

ACE_Atomic_Op<ACE_SYNCH_MUTEX, CORBA::Long> key(0);

// Only for Microsoft VC6
#if defined (_MSC_VER) && (_MSC_VER >= 1200) && (_MSC_VER < 1300)

// Added unused arguments with default value to work around with vc6
// bug on template function instantiation.
template<class DT, class DW, class DW_var>
::DDS::ReturnCode_t write (int writer_id,
                           ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> & timeout_writes,
                           ::DDS::DataWriter_ptr writer,
                           DT* dt = 0, DW* dw = 0, DW_var* dw_var = 0)
{
  ACE_UNUSED_ARG (dt);
  ACE_UNUSED_ARG (dw);
  ACE_UNUSED_ARG (dw_var);

#else

template<class DT, class DW, class DW_var>
::DDS::ReturnCode_t write (int writer_id,
                           ACE_Atomic_Op<ACE_SYNCH_MUTEX, int> & timeout_writes,
                           ::DDS::DataWriter_ptr writer)
{
#endif

  try
  {
    DT foo;
    //foo.data_source set below.
    foo.x = -1;
    foo.y = (float)writer_id;

    // Use the thread id as the instance key.
    foo.data_source = ++key;

    DW_var foo_dw
      = DW::_narrow(writer);
    TEST_CHECK (! CORBA::is_nil (foo_dw.in ()));

    ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("%T (%P|%t) Writer::svc starting to write.\n")));

    ::DDS::InstanceHandle_t handle
        = foo_dw->register_instance(foo);

    for (int i = 0; i< num_samples_per_instance; i ++)
    {
      foo.x = (float)i;

      foo.values.length (sequence_length);

      for (int j = 0; j < sequence_length; j ++)
      {
        foo.values[j] = (float) (i * i - j);
      }

      ::DDS::ReturnCode_t ret
        = foo_dw->write(foo,
                        handle);

      if (ret != ::DDS::RETCODE_OK)
      {
        ACE_ERROR ((LM_ERROR,
                    ACE_TEXT("(%P|%t)ERROR  Writer::svc, ")
                    ACE_TEXT ("%dth write() returned %d.\n"),
                    i, ret));
        if (ret == ::DDS::RETCODE_TIMEOUT)
        {
          timeout_writes ++;
        }
      }

      if (using_udp || using_mcast)
      {
        // Delay between sends to avoid dropping.
        static ACE_Time_Value delay(op_interval_ms/1000,
                                    op_interval_ms%1000 *1000);
        ACE_OS::sleep (delay);
      }

      // No delay necessary for reliable multicast
    }
  }
  catch (const CORBA::Exception& ex)
  {
    ex._tao_print_exception ("Exception caught in svc:");
  }

  return ::DDS::RETCODE_OK;
}


Writer::Writer(::DDS::DataWriter_ptr writer,
               int writer_id)
: writer_ (::DDS::DataWriter::_duplicate (writer)),
  writer_id_ (writer_id),
  finished_instances_ (0),
  timeout_writes_ (0)
{
}

void
Writer::start ()
{
  ACE_DEBUG((LM_DEBUG,
    ACE_TEXT("(%P|%t) Writer::start \n")));
  // Lanuch num_instances_per_writer threads.
  // Each thread writes one instance which uses the thread id as the
  // key value.
  if (activate (THR_NEW_LWP | THR_JOINABLE, num_instances_per_writer) == -1)
  {
    ACE_ERROR ((LM_ERROR,
                ACE_TEXT("(%P|%t) Writer::start, %p.\n"),
                ACE_TEXT("activate")));
    throw TestException ();
  }
}

void
Writer::end ()
{
  ACE_DEBUG((LM_DEBUG,
             ACE_TEXT("(%P|%t) Writer::end \n")));
  wait ();
}


int
Writer::svc ()
{
  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::svc begins.\n")));

  if (no_key)
  {
    write<Xyz::FooNoKey,
          ::Xyz::FooNoKeyDataWriter,
          ::Xyz::FooNoKeyDataWriter_var>
          (writer_id_, timeout_writes_, writer_.in ());
  }
  else
  {
    write<Xyz::Foo,
          ::Xyz::FooDataWriter,
          ::Xyz::FooDataWriter_var>
          (writer_id_, timeout_writes_, writer_.in ());
  }

  ACE_DEBUG((LM_DEBUG,
              ACE_TEXT("(%P|%t) Writer::svc finished.\n")));

  finished_instances_ ++;

  return 0;
}


long
Writer::writer_id () const
{
  return writer_id_;
}


bool
Writer::is_finished () const
{
  return finished_instances_ == num_instances_per_writer;
}

int
Writer::get_timeout_writes () const
{
  return timeout_writes_.value ();
}
