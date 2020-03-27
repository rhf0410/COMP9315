---------------------------------------------------------------------------
--
-- email.sql-
--    This file shows how to create a new user-defined type and how to
--    use this new type.
--
--
-- Portions Copyright (c) 1996-2018, PostgreSQL Global Development Group
-- Portions Copyright (c) 1994, Regents of the University of California
--
-- src/tutorial/email.source
--
---------------------------------------------------------------------------

-----------------------------
-- Creating a new type:
--	We are going to create a new type called 'email' which represents
--	email numbers.
--	A user-defined type must have an input and an output function, and
--	optionally can have binary input and output functions.  All of these
--	are usually user-defined C functions.
-----------------------------

-- Assume the user defined functions are in /srvr/z5184816/postgresql-11.3/src/tutorial/complex$DLSUFFIX
-- (we do not want to assume this is in the dynamic loader search path).
-- Look at $PWD/email.c for the source.  Note that we declare all of
-- them as STRICT, so we do not need to cope with NULL inputs in the
-- C code.  We also mark them IMMUTABLE, since they always return the
-- same outputs given the same inputs.

-- the input function 'email_in' takes a null-terminated string (the
-- textual representation of the type) and turns it into the internal
-- (in memory) representation. You will get a message telling you 'email'
-- does not exist yet but that's okay.

CREATE FUNCTION email_in(cstring)
   RETURNS emailaddr
   AS '/srvr/z5184816/postgresql-11.3/src/tutorial/email'
   LANGUAGE C IMMUTABLE STRICT;

-- the output function 'email_out' takes the internal representation and
-- converts it into the textual representation.

CREATE FUNCTION email_out(emailaddr)
   RETURNS cstring
   AS '/srvr/z5184816/postgresql-11.3/src/tutorial/email'
   LANGUAGE C IMMUTABLE STRICT;

-- the binary input function 'email_receive' takes a StringInfo buffer
-- and turns its contents into the internal representation.

CREATE FUNCTION email_receive(internal)
   RETURNS emailaddr
   AS '/srvr/z5184816/postgresql-11.3/src/tutorial/email'
   LANGUAGE C IMMUTABLE STRICT;

-- the binary output function 'email_send' takes the internal representation
-- and converts it into a (hopefully) platform-independent bytea string.

CREATE FUNCTION email_send(emailaddr)
   RETURNS bytea
   AS '/srvr/z5184816/postgresql-11.3/src/tutorial/email'
   LANGUAGE C IMMUTABLE STRICT;


-- now, we can create the type. The internallength specifies the size of the
-- memory block required to hold the type (we need two 8-byte doubles).

CREATE TYPE emailaddr (
   internallength = 256,
   input = email_in,
   output = email_out,
   receive = email_receive,
   send = email_send,
   alignment = char
);

-----------------------------
-- Interfacing New Types with Indexes:
--	We cannot define a secondary index (eg. a B-tree) over the new type
--	yet. We need to create all the required operators and support
--      functions, then we can make the operator class.
-----------------------------

-- first, define the required operators
CREATE FUNCTION email_lt(emailaddr, emailaddr) RETURNS bool
   AS '/srvr/z5184816/postgresql-11.3/src/tutorial/email' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION email_le(emailaddr, emailaddr) RETURNS bool
   AS '/srvr/z5184816/postgresql-11.3/src/tutorial/email' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION email_eq(emailaddr, emailaddr) RETURNS bool
   AS '/srvr/z5184816/postgresql-11.3/src/tutorial/email' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION email_neq(emailaddr, emailaddr) RETURNS bool
   AS '/srvr/z5184816/postgresql-11.3/src/tutorial/email' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION email_gt(emailaddr, emailaddr) RETURNS bool
   AS '/srvr/z5184816/postgresql-11.3/src/tutorial/email' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION email_ge(emailaddr, emailaddr) RETURNS bool
   AS '/srvr/z5184816/postgresql-11.3/src/tutorial/email' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION email_deq(emailaddr, emailaddr) RETURNS bool
   AS '/srvr/z5184816/postgresql-11.3/src/tutorial/email' LANGUAGE C IMMUTABLE STRICT;
CREATE FUNCTION email_ndeq(emailaddr, emailaddr) RETURNS bool
   AS '/srvr/z5184816/postgresql-11.3/src/tutorial/email' LANGUAGE C IMMUTABLE STRICT;

CREATE OPERATOR < (
   leftarg = emailaddr,
   rightarg = emailaddr,
   procedure = email_lt,
   commutator = > ,
   negator = >= ,
   restrict = scalarltsel,
   join = scalarltjoinsel
);

CREATE OPERATOR <= (
   leftarg = emailaddr,
   rightarg = emailaddr,
   procedure = email_le,
   commutator = >= ,
   negator = > ,
   restrict = scalarlesel,
   join = scalarlejoinsel
);

CREATE OPERATOR = (
   leftarg = emailaddr,
   rightarg = emailaddr,
   procedure = email_eq,
   commutator = = ,
   -- leave out negator since we didn't create <> operator
   negator = <> ,
   restrict = eqsel,
   join = eqjoinsel,
   HASHES,
   MERGES
);

CREATE OPERATOR <> (
   leftarg = emailaddr,
   rightarg = emailaddr,
   procedure = email_neq,
   commutator = <> ,
   -- leave out negator since we didn't create <> operator
   negator = = ,
   restrict = eqsel,
   join = eqjoinsel,
   HASHES,
   MERGES
);

CREATE OPERATOR >= (
   leftarg = emailaddr,
   rightarg = emailaddr,
   procedure = email_ge,
   commutator = <= ,
    negator = < ,
   restrict = scalargesel,
   join = scalargejoinsel
);

CREATE OPERATOR > (
   leftarg = emailaddr,
   rightarg = emailaddr,
   procedure = email_gt,
   commutator = < ,
   negator = <= ,
   restrict = scalargtsel,
   join = scalargtjoinsel
);

CREATE OPERATOR ~ (
   leftarg = emailaddr,
   rightarg = emailaddr,
   procedure = email_deq,
   commutator = ~ ,
   negator = !~ ,
   restrict = eqsel,
   join = eqjoinsel,
   HASHES,
   MERGES
);

CREATE OPERATOR !~ (
   leftarg = emailaddr,
   rightarg = emailaddr,
   procedure = email_ndeq,
   commutator = !~ ,
   negator = ~ ,
   restrict = neqsel,
   join = neqjoinsel,
   HASHES,
   MERGES
);

-- create the support function too
CREATE FUNCTION email_cmp(emailaddr, emailaddr) RETURNS int4
   AS '/srvr/z5184816/postgresql-11.3/src/tutorial/email' LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION email_hval(emailaddr) RETURNS int4
  AS '/srvr/z5184816/postgresql-11.3/src/tutorial/email' LANGUAGE C IMMUTABLE STRICT;

-- now we can make the operator class
CREATE OPERATOR CLASS emailaddr_abs_ops
    DEFAULT FOR TYPE emailaddr USING btree AS
        OPERATOR        1       < (emailaddr, emailaddr),
        OPERATOR        2       <= (emailaddr, emailaddr),
        OPERATOR        3       = (emailaddr, emailaddr),
        OPERATOR        4       >= (emailaddr, emailaddr),
        OPERATOR        5       > (emailaddr, emailaddr),
        FUNCTION        1       email_cmp(emailaddr, emailaddr);

CREATE OPERATOR CLASS emailaddress_hash_ops
    DEFAULT FOR TYPE emailaddr USING hash AS
        OPERATOR        1    =,
        FUNCTION        1    email_hval(emailaddr);
