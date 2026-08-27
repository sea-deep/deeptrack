-- DeepTrack's browser writes locally first, then upserts these immutable,
-- client-addressed records after connectivity returns. Demo telemetry is never
-- persisted. Every key includes user_id so foreign keys cannot cross tenants.

create table public.rover_sessions (
  user_id uuid not null references auth.users (id) on delete cascade,
  id text not null,
  rover_id text not null default 'DT-ALPHA-01',
  mode text not null default 'hardware',
  status text not null default 'active',
  started_at timestamptz not null,
  ended_at timestamptz,
  start_pose jsonb,
  end_pose jsonb,
  metadata jsonb not null default '{}'::jsonb,
  updated_at timestamptz not null,
  primary key (user_id, id),
  constraint rover_sessions_id_format check (
    id ~ '^sess_[0-9A-Z]{26}$'
  ),
  constraint rover_sessions_rover_id_length check (
    char_length(rover_id) between 1 and 64
  ),
  constraint rover_sessions_mode check (mode in ('hardware')),
  constraint rover_sessions_status check (
    status in ('active', 'closed', 'interrupted')
  ),
  constraint rover_sessions_time_order check (
    ended_at is null or ended_at >= started_at
  ),
  constraint rover_sessions_start_pose_object check (
    start_pose is null or jsonb_typeof(start_pose) = 'object'
  ),
  constraint rover_sessions_end_pose_object check (
    end_pose is null or jsonb_typeof(end_pose) = 'object'
  ),
  constraint rover_sessions_metadata_object check (
    jsonb_typeof(metadata) = 'object'
  )
);

create index rover_sessions_user_started_idx
  on public.rover_sessions (user_id, started_at desc);

create table public.mission_records (
  user_id uuid not null,
  id text not null,
  session_id text not null,
  record_type text not null,
  captured_at timestamptz not null,
  sequence bigint not null,
  payload jsonb not null,
  inserted_at timestamptz not null default now(),
  primary key (user_id, id),
  constraint mission_records_session_fkey
    foreign key (user_id, session_id)
    references public.rover_sessions (user_id, id)
    on delete cascade,
  constraint mission_records_id_format check (
    id ~ '^rec_[0-9A-Z]{26}$'
  ),
  constraint mission_records_type check (
    record_type in ('telemetry', 'event', 'scan', 'map_snapshot', 'sentinel')
  ),
  constraint mission_records_sequence check (sequence >= 0),
  constraint mission_records_payload_object check (
    jsonb_typeof(payload) = 'object'
  )
);

create index mission_records_user_captured_idx
  on public.mission_records (user_id, captured_at desc);

create index mission_records_session_captured_idx
  on public.mission_records (user_id, session_id, captured_at desc);

alter table public.rover_sessions enable row level security;
alter table public.mission_records enable row level security;

revoke all on table public.rover_sessions from anon, authenticated;
revoke all on table public.mission_records from anon, authenticated;

grant select, insert, update on table public.rover_sessions to authenticated;
grant select, insert on table public.mission_records to authenticated;

create policy "Users read their rover sessions"
  on public.rover_sessions
  for select
  to authenticated
  using ((select auth.uid()) = user_id);

create policy "Users create their rover sessions"
  on public.rover_sessions
  for insert
  to authenticated
  with check ((select auth.uid()) = user_id);

create policy "Users update their rover sessions"
  on public.rover_sessions
  for update
  to authenticated
  using ((select auth.uid()) = user_id)
  with check ((select auth.uid()) = user_id);

create policy "Users read their mission records"
  on public.mission_records
  for select
  to authenticated
  using ((select auth.uid()) = user_id);

create policy "Users append their mission records"
  on public.mission_records
  for insert
  to authenticated
  with check ((select auth.uid()) = user_id);

comment on table public.rover_sessions is
  'Authenticated, user-owned real-rover recording sessions.';
comment on table public.mission_records is
  'Append-only real-rover telemetry, events and map snapshots queued by the offline dashboard.';
