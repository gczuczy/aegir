import { NgModule } from '@angular/core';
import { RouterModule, Routes } from '@angular/router';

//import { PROGRAMS_ROUTES } from './programs/programs.routes';

import { BrewComponent } from './brew/brew.component';
import { ProgramsComponent } from './programs/programs.component';
import { SettingsComponent } from './settings/settings.component';
import { MaintenanceComponent } from './maintenance/maintenance.component';


const routes: Routes = [
  { path: '', redirectTo: 'programs', pathMatch: 'full' },
  { path: 'brew', component: BrewComponent },
//  { path: 'programs', component: ProgramsComponent,
//    children: PROGRAMS_ROUTES},
  { path: 'maintenance', component: MaintenanceComponent},
  { path: 'settings', component: SettingsComponent }
];

@NgModule({
  imports: [RouterModule.forRoot(routes)],
  exports: [RouterModule]
})
export class AppRoutingModule { }
