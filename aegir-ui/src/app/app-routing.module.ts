import { NgModule } from '@angular/core';
import { RouterModule, Routes } from '@angular/router';

import { BrewComponent } from './brew/brew.component';
import { ProgramsComponent } from './programs/programs.component';
import { SettingsComponent } from './settings/settings.component';
import { MaintenanceComponent } from './maintenance/maintenance.component';

import { ProgramComponent } from './program/program.component';
import { AddProgramComponent } from './add-program/add-program.component';
import { EditProgramComponent } from './edit-program/edit-program.component';
import { LoadProgramComponent } from './load-program/load-program.component';

export const PROGRAMS_ROUTES: Routes = [
  { path: '', redirectTo: 'add', pathMatch: 'full' },
  { path: 'add', component: AddProgramComponent },
  { path: ':id/edit', component: EditProgramComponent },
  { path: ':id/view', component: ProgramComponent },
  { path: ':id/load', component: LoadProgramComponent },
]

const routes: Routes = [
  { path: '', redirectTo: 'programs', pathMatch: 'full' },
  { path: 'brew', component: BrewComponent },
  { path: 'programs', component: ProgramsComponent ,
    children: PROGRAMS_ROUTES},
  { path: 'maintenance', component: MaintenanceComponent},
  { path: 'settings', component: SettingsComponent }
];

@NgModule({
  imports: [RouterModule.forRoot(routes)],
  exports: [RouterModule]
})
export class AppRoutingModule { }
