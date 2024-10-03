---

# Basic Data Types and Conventions {#chap-datatypes}

In this section we describe our assumptions and conventions regarding
the use of data types, variables and packet field names in DCP,
insofar as they are relevant beyond the scope of just one of the
component protocols. In addition, each of the constituent protocols of
DCP introduces its own data types, variables and field names.


## Naming and Typesetting Conventions {#chap-datatypes-conventions}


- We are going to be explicit about variables, packet field names and
  their respective types. Any variable, field name or type is typeset
  like `this`.

- We consistently start the names of variables or packet field names
  with a lower-case letter, like for example `length`, whereas types
  always start with an uppercase letter, like for example
  `String`. For the remainder of the name we often adopt the camel
  case convention.

  For data types that will be included in packets (referred to as
  **transmissible data types**, see below) we will sometimes indicate
  their width (as a number of bits) and, where relevant, their precise
  representation. In other cases, the width of a transmissible data
  type can be chosen by implementers as an integral multiple of one
  byte.  For data types not to be included in packets
  (**non-transmissible data types**) we will not provide any details
  about their representation and leave it to the implementation.


## Basic Non-Transmissible Data Types

The following basic data types are used inside an implementation but
are not to be included in any packet transmission. They are generally
system- and programming-language dependent.

- `String` is a data type for human-readable strings.
- `UInt` is a data type for unsigned integer values, having a
  system-dependent width of at least 32 bits.
- `Bool` is the Boolean data type with values `true` and `false`.


## Basic Transmissible Data Types

The following basic data types can be used inside an implementation
but are particularly included as fields in packets. The type names of
transmissible data types end with `T`, and their width, when not
explicitly given, is always required to be an integral multiple of one
byte. Where a transmissible data type requires two or more bytes, it
is transmitted in network byte order.

- Stations have unique identifiers, which we refer to as **node
  identifiers**, and these are of fixed-length data type
  `NodeIdentifierT` that is not specified here in detail. Such an
  identifier can be administratively assigned or the implementer
  chooses to use a data type already provided by the underlying
  wireless technology, like for example a 48-bit WiFi MAC address. A
  beacon sender includes its identfier in every beacon and a receiving
  node can extract that identifier from a received beacon to identify
  the sender. Furthermore, each of the DCP protocols is able to
  retrieve the own node identifier.

- Nodes have access to a source of physical time
  (e.g. GPS). Timestamps are represented using a fixed-length data
  type `TimeStampT` that is not specified here in detail. We expect
  that for the operations of the DCP/VarDis protocols an accuracy of
  within a few milliseconds will be sufficient, applications might
  require greater accuracy.

- When a human-readable string is to be transmitted, then the data
  type `StringT` is used. In this type, a string starts with one
  single byte indicating the length of the string (hence having a
  maximum length of 255), followed by as many bytes as indicated by
  the length field. When variable-length encodings such as UTF-8 are
  used for strings, then the length of the string may be larger than
  the number of characters displayed to human users.



## Queues and Lists {#sec-queues-lists}

In some places the DCP constituent protocols need to keep runtime data
organized in (first-in-first-out) queues or lists. As is common in
modern programming languages, we treat queues and lists as abstract
parameterizable data types that can have elements of a particular
element type `T`. We refer to the type of a queue with element type
`T` by the type name `Queue<T>`, and similarly we refer to the type of
a list made up of elements of type `T` as `List<T>`.

For the purposes of this specification, the `Queue` data type
generally supports the following operations:

- `qTake()` removes the head-of-line element from the queue and
  returns it (or signals that the queue is empty). It does not 
  take any parameters.
- `qPeek()` returns the value of the head-of-queue element, but
  without removing it from the queue (or signals that the queue is
  empty). It does not take any parameters and does not modify the
  queue.
- `qAppend()` takes a value of the element type `T` as a parameter and
  appends it to the end of the queue.
- `qRemove()` takes a value of element type `T` as a parameter and
  removes all elements with the same value from the queue.
- `qIsEmpty()` returns a value of type `Bool`  telling whether or not
  the queue is empty. It does not take any parameters and does not
  modify the queue.
- `qExists()` takes a value of element type `T` as parameter and
  returns `true` if the queue contains an entry with value equal to
  the parameter, and `false` otherwise.
- `qLength()` takes no parameters and returns the number of elements
  currently stored in the queue.


The `List` data type generally supports the following operations:

- `lInsert()` inserts a new element of element type `T` into a list.
- `lRemove()` takes a value of element type `T` as a parameter and
  removes all elements with the same value from the list.
- `lIsEmpty()` returns a value of type `Bool` telling whether or not
  the list is empty. It does not take any parameters and does not
  modify the list.
- `lExists()` takes a value of element type `T` as parameter and
  returns `true` if the list contains an entry with value equal to
  the parameter, and `false` otherwise.
- `lLength()` takes no parameters and returns the number of elements
  currently stored in the list.
  

## Other Conventions

- We will interchangeably refer to drones, nodes and stations.

- We will specify interfaces offered by protocols through generic
  service primitives, i.e. for a particular service `S` there might be
  a `request`, a `confirm` or an `indication` primitive.  For every
  `S.request` primitive a matching `S.confirm` primitive shall be
  returned, and at minimum the `S.confirm` primitive carries a status
  parameter indicating the outcome of processing the `S.request`
  primitive. We do not prescribe the precise mechanism through which
  service primitives are being exchanged, this is
  implementation-dependent. However, when describing the processing of
  a `S.request` primitive for a service `S`, we will frequently use
  the keyword `return`, followed by some status code. This indicates
  that processing of the `S.request` primitive stops and that a
  `S.confirm` primitive carrying the indicated status code shall be
  returned to the entity generating the `S.request` primitive.

- We use pseudo-code to describe parts of the behaviour of DCP/VarDis
  protocols. Syntactically, in this pseudo-code we borrow some
  elements of the C++ programming language.


