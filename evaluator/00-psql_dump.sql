\c data

create table if not exists result (
  method                  char    (64) not null,
  input                   text         not null,
  size                    bigint       not null,
  threshold               integer      not null,
  wall_nsecs              bigint       not null,
  user_nsecs              bigint       not null,
  system_nsecs            bigint       not null,
  hw_cpu_cycles           bigint       not null,
  hw_instructions         bigint       not null,
  hw_cache_references     bigint       not null,
  hw_cache_misses         bigint       not null,
  hw_branch_instructions  bigint       not null,
  hw_branch_misses        bigint       not null,
  hw_bus_cycles           bigint       not null,
  sw_cpu_clock            bigint       not null,
  sw_task_clock           bigint       not null,
  sw_page_faults          bigint       not null,
  sw_context_switches     bigint       not null,
  sw_cpu_migrations       bigint       not null,
  id                      integer      not null,
  description             char    (64) not null,
  run_type                char    (64) not null
);
