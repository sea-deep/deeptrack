begin;

create extension if not exists pgtap with schema extensions;

select plan(14);

select has_table('public', 'rover_sessions', 'rover_sessions exists');
select has_table('public', 'mission_records', 'mission_records exists');

select ok(
  (select relrowsecurity from pg_class
   where oid = 'public.rover_sessions'::regclass),
  'rover_sessions has RLS enabled'
);
select ok(
  (select relrowsecurity from pg_class
   where oid = 'public.mission_records'::regclass),
  'mission_records has RLS enabled'
);

select is(
  (select count(*)::integer from pg_policies
   where schemaname = 'public' and tablename = 'rover_sessions'),
  3,
  'rover_sessions has separate select, insert and update policies'
);
select is(
  (select count(*)::integer from pg_policies
   where schemaname = 'public' and tablename = 'mission_records'),
  2,
  'mission_records has separate select and insert policies'
);

select ok(
  has_table_privilege('authenticated', 'public.rover_sessions', 'select'),
  'authenticated users can read sessions through RLS'
);
select ok(
  has_table_privilege('authenticated', 'public.rover_sessions', 'update'),
  'authenticated users can close their own sessions'
);
select ok(
  not has_table_privilege('authenticated', 'public.rover_sessions', 'delete'),
  'authenticated users cannot delete sessions'
);
select ok(
  not has_table_privilege('anon', 'public.rover_sessions', 'select'),
  'anonymous users cannot read sessions'
);
select ok(
  has_table_privilege('authenticated', 'public.mission_records', 'insert'),
  'authenticated users can append records through RLS'
);
select ok(
  not has_table_privilege('authenticated', 'public.mission_records', 'update'),
  'mission records are append-only'
);

select has_index(
  'public', 'rover_sessions', 'rover_sessions_user_started_idx',
  'session history lookup is indexed'
);
select has_index(
  'public', 'mission_records', 'mission_records_session_captured_idx',
  'session record lookup is indexed'
);

select * from finish();
rollback;
