import { Routes } from '@angular/router';

import { BrewComponent } from './brew/brew.component';
import { ProgramComponent } from './program/program.component';
import { SettingsComponent } from './settings/settings.component';

export const APP_ROUTES: Routes = [
    { path: '', redirectTo: 'programs', pathMatch: 'full' },
    { path: 'brew', component: BrewComponent },
    { path: 'program', component: ProgramComponent },
    { path: 'settings', component: SettingsComponent }
]
